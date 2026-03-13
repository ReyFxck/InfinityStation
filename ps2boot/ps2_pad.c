#include "ps2_pad.h"

#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <libpad.h>

#include "libretro.h"

static int g_pad_inited = 0;
static int g_pad_ready = 0;
static int g_pad_opened = 0;

static char g_pad_buf[256] __attribute__((aligned(64)));
static struct padButtonStatus g_pad_status;
static uint32_t g_buttons = 0;

int ps2_pad_init_once(void)
{
    int ret_sio2 = -1;
    int ret_padman = -1;

    if (g_pad_inited)
        return g_pad_ready;

    g_pad_inited = 1;

    SifLoadFileInit();

    ret_sio2 = SifLoadModule("rom0:SIO2MAN", 0, NULL);
    if (ret_sio2 < 0)
        ret_sio2 = SifLoadModule("rom0:XSIO2MAN", 0, NULL);

    ret_padman = SifLoadModule("rom0:PADMAN", 0, NULL);
    if (ret_padman < 0)
        ret_padman = SifLoadModule("rom0:XPADMAN", 0, NULL);

    SifLoadFileExit();

    if (ret_sio2 < 0 || ret_padman < 0)
        return 0;

    if (padInit(0) != 1)
        return 0;

    if (!padPortOpen(0, 0, g_pad_buf))
        return 0;

    g_pad_opened = 1;
    g_pad_ready = 1;
    return 1;
}

void ps2_pad_poll(void)
{
    int state;
    uint32_t now;

    if (!g_pad_opened)
        return;

    state = padGetState(0, 0);

    if (state != PAD_STATE_STABLE && state != PAD_STATE_FINDCTP1)
        return;

    if (!padRead(0, 0, &g_pad_status))
        return;

    now = 0xffff ^ g_pad_status.btns;
    g_buttons = now;
}

uint32_t ps2_pad_buttons(void)
{
    return g_buttons;
}

int ps2_pad_ready(void)
{
    return g_pad_ready;
}

int16_t ps2_pad_libretro_state(unsigned id)
{
    uint32_t b = g_buttons;

    switch (id) {
        case RETRO_DEVICE_ID_JOYPAD_B:      return (b & PAD_CROSS)    ? 1 : 0;
        case RETRO_DEVICE_ID_JOYPAD_Y:      return (b & PAD_SQUARE)   ? 1 : 0;
        case RETRO_DEVICE_ID_JOYPAD_SELECT: return (b & PAD_SELECT)   ? 1 : 0;
        case RETRO_DEVICE_ID_JOYPAD_START:  return (b & (PAD_START | PAD_CROSS)) ? 1 : 0;
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
