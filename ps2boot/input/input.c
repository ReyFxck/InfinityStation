#include "input.h"

#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <libpad.h>
#include <libmtap.h>
#include <ps2_joystick_driver.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "libretro.h"

typedef struct ps2_input_route
{
    unsigned char valid;
    unsigned char phys_port;
    unsigned char phys_slot;
} ps2_input_route_t;

static int g_input_inited = 0;
static int g_input_ready  = 0;

static int g_mtap_ready = 0;
static int g_mtap_port_open[PS2_INPUT_MAX_PORTS] = {0};
static int g_mtap_connected[PS2_INPUT_MAX_PORTS] = {0};
static int g_slot_count[PS2_INPUT_MAX_PORTS] = {0};

static int g_pad_opened[PS2_INPUT_MAX_PORTS][PS2_INPUT_MAX_SLOTS] = {{0}};
static int g_analog_checked[PS2_INPUT_MAX_PORTS][PS2_INPUT_MAX_SLOTS] = {{0}};
static int g_analog_supported[PS2_INPUT_MAX_PORTS][PS2_INPUT_MAX_SLOTS] = {{0}};
static int g_analog_requested[PS2_INPUT_MAX_PORTS][PS2_INPUT_MAX_SLOTS] = {{0}};

static char g_pad_buf[PS2_INPUT_MAX_PORTS][PS2_INPUT_MAX_SLOTS][256] __attribute__((aligned(64)));
static struct padButtonStatus g_pad_status[PS2_INPUT_MAX_PORTS][PS2_INPUT_MAX_SLOTS];

static uint32_t g_buttons[PS2_INPUT_MAX_PORTS][PS2_INPUT_MAX_SLOTS] = {{0}};
static uint32_t g_buttons_raw[PS2_INPUT_MAX_PORTS][PS2_INPUT_MAX_SLOTS] = {{0}};

static ps2_input_route_t g_player_routes[PS2_INPUT_MAX_PLAYERS];

#define PS2_ANALOG_MODE_ID 7
#define PS2_STICK_CENTER   0x80
#define PS2_STICK_DEADZONE 0x28

static int ps2_input_port_valid(unsigned port)
{
    return port < PS2_INPUT_MAX_PORTS;
}

static int ps2_input_slot_valid(unsigned slot)
{
    return slot < PS2_INPUT_MAX_SLOTS;
}

static int ps2_input_player_valid(unsigned player)
{
    return player < PS2_INPUT_MAX_PLAYERS;
}

static void ps2_input_reset_slot_state(unsigned port, unsigned slot)
{
    if (!ps2_input_port_valid(port) || !ps2_input_slot_valid(slot))
        return;

    memset(&g_pad_status[port][slot], 0, sizeof(g_pad_status[port][slot]));
    g_buttons[port][slot] = 0;
    g_buttons_raw[port][slot] = 0;
    g_analog_checked[port][slot] = 0;
    g_analog_supported[port][slot] = 0;
    g_analog_requested[port][slot] = 0;
}

static void ps2_input_clear_routes(void)
{
    memset(g_player_routes, 0, sizeof(g_player_routes));
}

static void ps2_input_map_player(unsigned player, unsigned phys_port, unsigned phys_slot)
{
    if (!ps2_input_player_valid(player) ||
        !ps2_input_port_valid(phys_port) ||
        !ps2_input_slot_valid(phys_slot))
        return;

    if (!g_pad_opened[phys_port][phys_slot])
        return;

    g_player_routes[player].valid = 1;
    g_player_routes[player].phys_port = (unsigned char)phys_port;
    g_player_routes[player].phys_slot = (unsigned char)phys_slot;
}

static int ps2_input_resolve_player(unsigned player, unsigned *phys_port, unsigned *phys_slot)
{
    if (!ps2_input_player_valid(player))
        return 0;

    if (!g_player_routes[player].valid)
        return 0;

    if (phys_port)
        *phys_port = g_player_routes[player].phys_port;
    if (phys_slot)
        *phys_slot = g_player_routes[player].phys_slot;

    return 1;
}

static void ps2_input_rebuild_routes(void)
{
    unsigned player = 2;
    unsigned phys_port;
    unsigned phys_slot;

    ps2_input_clear_routes();

    ps2_input_map_player(0, 0, 0);
    ps2_input_map_player(1, 1, 0);

    for (phys_port = 0; phys_port < PS2_INPUT_MAX_PORTS && player < PS2_INPUT_MAX_PLAYERS; phys_port++) {
        for (phys_slot = 1; phys_slot < PS2_INPUT_MAX_SLOTS && player < PS2_INPUT_MAX_PLAYERS; phys_slot++) {
            if (!g_pad_opened[phys_port][phys_slot])
                continue;

            ps2_input_map_player(player, phys_port, phys_slot);
            player++;
        }
    }
}

static int ps2_input_open_slot(unsigned port, unsigned slot)
{
    if (!ps2_input_port_valid(port) || !ps2_input_slot_valid(slot))
        return 0;

    if (g_pad_opened[port][slot])
        return 1;

    ps2_input_reset_slot_state(port, slot);

    if (!padPortOpen((int)port, (int)slot, g_pad_buf[port][slot]))
        return 0;

    g_pad_opened[port][slot] = 1;
    return 1;
}

static void ps2_input_close_slot(unsigned port, unsigned slot)
{
    if (!ps2_input_port_valid(port) || !ps2_input_slot_valid(slot))
        return;

    if (g_pad_opened[port][slot]) {
        (void)padPortClose((int)port, (int)slot);
        g_pad_opened[port][slot] = 0;
    }

    ps2_input_reset_slot_state(port, slot);
}

static unsigned ps2_input_query_slot_count(unsigned port, int mtap_connected)
{
    int count;

    if (!mtap_connected)
        return 1u;

    count = padGetSlotMax((int)port);
    if (count < 2)
        count = 4;
    if (count > (int)PS2_INPUT_MAX_SLOTS)
        count = (int)PS2_INPUT_MAX_SLOTS;

    return (unsigned)count;
}

static void ps2_input_refresh_topology(int force)
{
    unsigned port;
    int changed = force ? 1 : 0;

    for (port = 0; port < PS2_INPUT_MAX_PORTS; port++) {
        int desired_connected = 0;
        unsigned desired_slots = 1u;
        unsigned slot;

        if (g_mtap_ready && g_mtap_port_open[port]) {
            desired_connected = (mtapGetConnection((int)port) == 1);
            desired_slots = ps2_input_query_slot_count(port, desired_connected);
        }

        if (force ||
            desired_connected != g_mtap_connected[port] ||
            desired_slots != (unsigned)g_slot_count[port]) {
            changed = 1;

            for (slot = desired_slots; slot < PS2_INPUT_MAX_SLOTS; slot++)
                ps2_input_close_slot(port, slot);

            for (slot = 0; slot < desired_slots; slot++)
                (void)ps2_input_open_slot(port, slot);

            g_mtap_connected[port] = desired_connected;
            g_slot_count[port] = (int)desired_slots;
        }
    }

    if (changed)
        ps2_input_rebuild_routes();
}

static void ps2_input_try_enable_analog(unsigned port, unsigned slot)
{
    int modes;
    int i;

    if (!ps2_input_port_valid(port) ||
        !ps2_input_slot_valid(slot) ||
        !g_pad_opened[port][slot])
        return;

    if (!g_analog_checked[port][slot]) {
        modes = padInfoMode((int)port, (int)slot, PAD_MODETABLE, -1);
        g_analog_checked[port][slot] = 1;
        g_analog_supported[port][slot] = 0;

        for (i = 0; i < modes; i++) {
            if (padInfoMode((int)port, (int)slot, PAD_MODETABLE, i) == PS2_ANALOG_MODE_ID) {
                g_analog_supported[port][slot] = 1;
                break;
            }
        }
    }

    if (!g_analog_supported[port][slot])
        return;

    if (!g_analog_requested[port][slot]) {
        padSetMainMode((int)port, (int)slot, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);
        g_analog_requested[port][slot] = 1;
    }
}

static int ps2_input_is_analog_active(unsigned port, unsigned slot)
{
    int cur_mode;

    if (!ps2_input_port_valid(port) ||
        !ps2_input_slot_valid(slot) ||
        !g_analog_supported[port][slot])
        return 0;

    cur_mode = padInfoMode((int)port, (int)slot, PAD_MODECURID, 0);
    return (cur_mode == PS2_ANALOG_MODE_ID);
}

static void ps2_input_apply_left_stick_as_dpad(unsigned port, unsigned slot, uint32_t *buttons_now)
{
    unsigned char lx;
    unsigned char ly;

    if (!ps2_input_port_valid(port) ||
        !ps2_input_slot_valid(slot) ||
        !buttons_now)
        return;

    lx = g_pad_status[port][slot].ljoy_h;
    ly = g_pad_status[port][slot].ljoy_v;

    if (lx + PS2_STICK_DEADZONE < PS2_STICK_CENTER)
        *buttons_now |= PAD_LEFT;
    else if (lx > PS2_STICK_CENTER + PS2_STICK_DEADZONE)
        *buttons_now |= PAD_RIGHT;

    if (ly + PS2_STICK_DEADZONE < PS2_STICK_CENTER)
        *buttons_now |= PAD_UP;
    else if (ly > PS2_STICK_CENTER + PS2_STICK_DEADZONE)
        *buttons_now |= PAD_DOWN;
}

static uint16_t ps2_input_libretro_mask_from_buttons(uint32_t b)
{
    uint16_t mask = 0;

    if (b & PAD_CROSS)    mask |= (uint16_t)(1u << RETRO_DEVICE_ID_JOYPAD_B);
    if (b & PAD_SQUARE)   mask |= (uint16_t)(1u << RETRO_DEVICE_ID_JOYPAD_Y);
    if (b & PAD_SELECT)   mask |= (uint16_t)(1u << RETRO_DEVICE_ID_JOYPAD_SELECT);
    if (b & PAD_START)    mask |= (uint16_t)(1u << RETRO_DEVICE_ID_JOYPAD_START);
    if (b & PAD_UP)       mask |= (uint16_t)(1u << RETRO_DEVICE_ID_JOYPAD_UP);
    if (b & PAD_DOWN)     mask |= (uint16_t)(1u << RETRO_DEVICE_ID_JOYPAD_DOWN);
    if (b & PAD_LEFT)     mask |= (uint16_t)(1u << RETRO_DEVICE_ID_JOYPAD_LEFT);
    if (b & PAD_RIGHT)    mask |= (uint16_t)(1u << RETRO_DEVICE_ID_JOYPAD_RIGHT);
    if (b & PAD_CIRCLE)   mask |= (uint16_t)(1u << RETRO_DEVICE_ID_JOYPAD_A);
    if (b & PAD_TRIANGLE) mask |= (uint16_t)(1u << RETRO_DEVICE_ID_JOYPAD_X);
    if (b & PAD_L1)       mask |= (uint16_t)(1u << RETRO_DEVICE_ID_JOYPAD_L);
    if (b & PAD_R1)       mask |= (uint16_t)(1u << RETRO_DEVICE_ID_JOYPAD_R);
    if (b & PAD_L2)       mask |= (uint16_t)(1u << RETRO_DEVICE_ID_JOYPAD_L2);
    if (b & PAD_R2)       mask |= (uint16_t)(1u << RETRO_DEVICE_ID_JOYPAD_R2);

    return mask;
}

void ps2_input_shutdown(void)
{
    unsigned port;
    unsigned slot;

    for (port = 0; port < PS2_INPUT_MAX_PORTS; port++) {
        for (slot = 0; slot < PS2_INPUT_MAX_SLOTS; slot++)
            ps2_input_close_slot(port, slot);

        if (g_mtap_ready && g_mtap_port_open[port]) {
            (void)mtapPortClose((int)port);
            g_mtap_port_open[port] = 0;
        }

        g_mtap_connected[port] = 0;
        g_slot_count[port] = 0;
    }

    if (g_input_inited)
        padEnd();

    ps2_input_clear_routes();

    g_mtap_ready = 0;
    g_input_inited = 0;
    g_input_ready = 0;
}

int ps2_input_init_once(void)
{
    int ret_sio2 = -1;
    int ret_pad  = -1;
    int ret_mtap = -1;
    int ret_pi   = -1;
    int ret_mi   = -1;
    unsigned port;

    if (g_input_inited)
        return g_input_ready;

    g_input_inited = 1;

    SifLoadFileInit();
    ret_sio2 = SifLoadModule("rom0:XSIO2MAN", 0, NULL);
    ret_pad  = SifLoadModule("rom0:XPADMAN", 0, NULL);
    ret_mtap = SifLoadModule("rom0:XMTAPMAN", 0, NULL);
    SifLoadFileExit();

    if (ret_sio2 < 0 || ret_pad < 0)
        return 0;

    ret_pi = padInit(0);
    if (ret_pi != 1)
        return 0;

    if (ret_mtap >= 0) {
        ret_mi = mtapInit();
        if (ret_mi == 1) {
            g_mtap_ready = 1;
            for (port = 0; port < PS2_INPUT_MAX_PORTS; port++) {
                if (mtapPortOpen((int)port) == 1)
                    g_mtap_port_open[port] = 1;
            }
        }
    }

    g_input_ready = 0;

    for (port = 0; port < PS2_INPUT_MAX_PORTS; port++) {
        g_slot_count[port] = 1;
        if (ps2_input_open_slot(port, 0))
            g_input_ready = 1;
    }

    ps2_input_refresh_topology(1);
    return g_input_ready;
}

void ps2_input_poll(void)
{
    unsigned port;
    unsigned slot;

    ps2_input_refresh_topology(0);

    for (port = 0; port < PS2_INPUT_MAX_PORTS; port++) {
        unsigned max_slots = (unsigned)g_slot_count[port];

        if (max_slots == 0)
            max_slots = 1;
        if (max_slots > PS2_INPUT_MAX_SLOTS)
            max_slots = PS2_INPUT_MAX_SLOTS;

        for (slot = 0; slot < max_slots; slot++) {
            int state;
            uint32_t now;
            const unsigned char *bb;

            if (!g_pad_opened[port][slot])
                continue;

            state = padGetState((int)port, (int)slot);

            if (state != PAD_STATE_STABLE && state != PAD_STATE_FINDCTP1) {
                g_buttons[port][slot] = 0;
                g_buttons_raw[port][slot] = 0;
                continue;
            }

            ps2_input_try_enable_analog(port, slot);

            if (!padRead((int)port, (int)slot, &g_pad_status[port][slot])) {
                g_buttons[port][slot] = 0;
                g_buttons_raw[port][slot] = 0;
                continue;
            }

            bb = (const unsigned char *)&g_pad_status[port][slot].btns;
            g_buttons_raw[port][slot] = 0xffff ^ (((uint32_t)bb[1] << 8) | (uint32_t)bb[0]);
            now = g_buttons_raw[port][slot];

            if (ps2_input_is_analog_active(port, slot))
                ps2_input_apply_left_stick_as_dpad(port, slot, &now);

            g_buttons[port][slot] = now;
        }
    }
}

uint32_t ps2_input_buttons(void)
{
    return ps2_input_buttons_port(0);
}

uint32_t ps2_input_buttons_port(unsigned port)
{
    unsigned phys_port;
    unsigned phys_slot;

    if (!ps2_input_resolve_player(port, &phys_port, &phys_slot))
        return 0;

    return g_buttons[phys_port][phys_slot];
}

uint32_t ps2_input_raw_buttons(void)
{
    return ps2_input_raw_buttons_port(0);
}

uint32_t ps2_input_raw_buttons_port(unsigned port)
{
    unsigned phys_port;
    unsigned phys_slot;

    if (!ps2_input_resolve_player(port, &phys_port, &phys_slot))
        return 0;

    return g_buttons_raw[phys_port][phys_slot];
}

unsigned char ps2_input_left_x(void)
{
    return ps2_input_left_x_port(0);
}

unsigned char ps2_input_left_x_port(unsigned port)
{
    unsigned phys_port;
    unsigned phys_slot;

    if (!ps2_input_resolve_player(port, &phys_port, &phys_slot))
        return PS2_STICK_CENTER;

    return g_pad_status[phys_port][phys_slot].ljoy_h;
}

unsigned char ps2_input_left_y(void)
{
    return ps2_input_left_y_port(0);
}

unsigned char ps2_input_left_y_port(unsigned port)
{
    unsigned phys_port;
    unsigned phys_slot;

    if (!ps2_input_resolve_player(port, &phys_port, &phys_slot))
        return PS2_STICK_CENTER;

    return g_pad_status[phys_port][phys_slot].ljoy_v;
}

int16_t ps2_input_libretro_state(unsigned port, unsigned id)
{
    uint32_t b = ps2_input_buttons_port(port);

    switch (id) {
        case RETRO_DEVICE_ID_JOYPAD_MASK:   return (int16_t)ps2_input_libretro_mask_from_buttons(b);
        case RETRO_DEVICE_ID_JOYPAD_B:      return (b & PAD_CROSS)    ? 1 : 0;
        case RETRO_DEVICE_ID_JOYPAD_Y:      return (b & PAD_SQUARE)   ? 1 : 0;
        case RETRO_DEVICE_ID_JOYPAD_SELECT: return (b & PAD_SELECT)   ? 1 : 0;
        case RETRO_DEVICE_ID_JOYPAD_START:  return (b & PAD_START)    ? 1 : 0;
        case RETRO_DEVICE_ID_JOYPAD_UP:     return (b & PAD_UP)       ? 1 : 0;
        case RETRO_DEVICE_ID_JOYPAD_DOWN:   return (b & PAD_DOWN)     ? 1 : 0;
        case RETRO_DEVICE_ID_JOYPAD_LEFT:   return (b & PAD_LEFT)     ? 1 : 0;
        case RETRO_DEVICE_ID_JOYPAD_RIGHT:  return (b & PAD_RIGHT)    ? 1 : 0;
        case RETRO_DEVICE_ID_JOYPAD_A:      return (b & PAD_CIRCLE)   ? 1 : 0;
        case RETRO_DEVICE_ID_JOYPAD_X:      return (b & PAD_TRIANGLE) ? 1 : 0;
        case RETRO_DEVICE_ID_JOYPAD_L:      return (b & PAD_L1)       ? 1 : 0;
        case RETRO_DEVICE_ID_JOYPAD_R:      return (b & PAD_R1)       ? 1 : 0;
        case RETRO_DEVICE_ID_JOYPAD_L2:     return (b & PAD_L2)       ? 1 : 0;
        case RETRO_DEVICE_ID_JOYPAD_R2:     return (b & PAD_R2)       ? 1 : 0;
        default:                            return 0;
    }
}
