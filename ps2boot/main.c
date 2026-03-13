#include <kernel.h>
#include <sifrpc.h>
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <libpad.h>

#include "libretro.h"
#include "ps2_video.h"
#include "ps2_input.h"
#include "ps2_menu.h"

extern unsigned char smw_sfc_start[];
extern unsigned char smw_sfc_end[];

#define AUTO_INPUT_TEST 0
#define DEBUG_OVERLAY 0

static unsigned g_frame_count = 0;
static unsigned g_input_tick = 0;
static enum retro_pixel_format g_pixel_format = RETRO_PIXEL_FORMAT_RGB565;
static uint32_t g_prev_buttons = 0;
static unsigned g_fps_display = 0;
static unsigned g_fps_accum = 0;
static clock_t g_fps_last_clock = 0;
static double g_core_nominal_fps = 60.0;
static clock_t g_frame_deadline = 0;

static void die(const char *msg)
{
    scr_printf("\n[ERRO] %s\n", msg);
    SleepThread();
    while (1) {}
}

static int auto_hold(unsigned tick, unsigned a, unsigned b)
{
    return (tick >= a && tick < b);
}

static double target_limit_fps(void)
{
    int mode = ps2_menu_frame_limit_mode();

    if (mode == PS2_FRAME_LIMIT_50)
        return 50.0;
    if (mode == PS2_FRAME_LIMIT_60)
        return 60.0;
    if (mode == PS2_FRAME_LIMIT_OFF)
        return 0.0;

    if (g_core_nominal_fps > 1.0)
        return g_core_nominal_fps;

    return 60.0;
}

static void throttle_frame_if_needed(void)
{
    double fps = target_limit_fps();
    double ticks_per_frame_d;
    clock_t ticks_per_frame;
    clock_t now;

    if (fps <= 0.0)
        return;

    ticks_per_frame_d = (double)CLOCKS_PER_SEC / fps;
    if (ticks_per_frame_d < 1.0)
        ticks_per_frame_d = 1.0;

    ticks_per_frame = (clock_t)(ticks_per_frame_d + 0.5);
    now = clock();

    if (g_frame_deadline == 0) {
        g_frame_deadline = now + ticks_per_frame;
        return;
    }

    if (now < g_frame_deadline) {
        while ((now = clock()) < g_frame_deadline) {}
    } else if ((now - g_frame_deadline) > (ticks_per_frame * 4)) {
        g_frame_deadline = now;
    }

    g_frame_deadline += ticks_per_frame;
}

static void update_fps_overlay(void)
{
    clock_t now = clock();
    double target = target_limit_fps();
    char l1[32];
    char l2[32];

    g_fps_accum++;

    if (g_fps_last_clock == 0)
        g_fps_last_clock = now;

    if ((now - g_fps_last_clock) >= CLOCKS_PER_SEC) {
        g_fps_display = (unsigned)(((double)g_fps_accum * (double)CLOCKS_PER_SEC) / (double)(now - g_fps_last_clock) + 0.5);
        g_fps_accum = 0;
        g_fps_last_clock = now;
    }

    if (ps2_menu_show_fps_enabled()) {
        if (target > 0.0)
            snprintf(l1, sizeof(l1), "FPS: %u/%.0f", g_fps_display, target);
        else
            snprintf(l1, sizeof(l1), "FPS: %u/OFF", g_fps_display);

        if (ps2_menu_frame_limit_mode() == PS2_FRAME_LIMIT_AUTO)
            snprintf(l2, sizeof(l2), "LIMIT: AUTO");
        else if (ps2_menu_frame_limit_mode() == PS2_FRAME_LIMIT_50)
            snprintf(l2, sizeof(l2), "LIMIT: 50");
        else if (ps2_menu_frame_limit_mode() == PS2_FRAME_LIMIT_60)
            snprintf(l2, sizeof(l2), "LIMIT: 60");
        else
            snprintf(l2, sizeof(l2), "LIMIT: OFF");

        ps2_video_set_debug(l1, l2, "", "");
    } else {
        ps2_video_set_debug("", "", "", "");
    }
}

static bool environ_cb(unsigned cmd, void *data)
{
    switch (cmd) {
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        g_pixel_format = *(const enum retro_pixel_format *)data;
        scr_printf("[ENV] pixel format = %d\n", g_pixel_format);
        return true;

    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: {
        const char **dir = (const char **)data;
        *dir = "";
        return true;
    }

    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY: {
        const char **dir = (const char **)data;
        *dir = "";
        return true;
    }

    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
    case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
    case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
    case RETRO_ENVIRONMENT_SET_GEOMETRY:
    case RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO:
    case RETRO_ENVIRONMENT_SET_VARIABLES:
        return true;

    case RETRO_ENVIRONMENT_GET_VARIABLE:
        return false;

    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        return false;

    default:
        return false;
    }
}

static void video_cb(const void *data, unsigned width, unsigned height, size_t pitch)
{
#if DEBUG_OVERLAY
    char l1[32], l2[32], l3[32], l4[32];
#endif

    g_frame_count++;

#if DEBUG_OVERLAY
    if ((g_frame_count % 30) == 0 || g_frame_count == 1) {
        snprintf(l1, sizeof(l1), "PAD=%04X", (unsigned)ps2_input_buttons());
        snprintf(l2, sizeof(l2), "%ux%u", width, height);
        snprintf(l3, sizeof(l3), "P=%u", (unsigned)pitch);
        snprintf(l4, sizeof(l4), "FMT=%d", g_pixel_format);
        ps2_video_set_debug(l1, l2, l3, l4);
    }
#endif

    update_fps_overlay();

    if (g_pixel_format == RETRO_PIXEL_FORMAT_RGB565)
        ps2_video_present_rgb565(data, width, height, pitch);
}

static void audio_cb(int16_t left, int16_t right)
{
    (void)left;
    (void)right;
}

static size_t audio_batch_cb(const int16_t *data, size_t frames)
{
    (void)data;
    return frames;
}

static void input_poll_cb(void)
{
    g_input_tick++;
}

static int16_t input_state_cb(unsigned port, unsigned device, unsigned index, unsigned id)
{
    (void)port;
    (void)device;
    (void)index;

#if AUTO_INPUT_TEST
    if (id == RETRO_DEVICE_ID_JOYPAD_START) {
        if (auto_hold(g_input_tick, 120, 180)) return 1;
        if (auto_hold(g_input_tick, 300, 360)) return 1;
    }
    if (id == RETRO_DEVICE_ID_JOYPAD_A) {
        if (auto_hold(g_input_tick, 300, 360)) return 1;
    }
    if (id == RETRO_DEVICE_ID_JOYPAD_RIGHT) {
        if (auto_hold(g_input_tick, 500, 620)) return 1;
    }
    if (id == RETRO_DEVICE_ID_JOYPAD_B) {
        if (auto_hold(g_input_tick, 650, 720)) return 1;
    }
    return 0;
#else
    return ps2_input_libretro_state(id);
#endif
}

int main(int argc, char *argv[])
{
    struct retro_system_info info;
    struct retro_game_info game;
    struct retro_system_av_info av;
    size_t rom_size;

    (void)argc;
    (void)argv;

    SifInitRpc(0);
    init_scr();

    scr_printf("ps2snes2005 embedded rom boot test\n");

    if (!ps2_video_init_once())
        die("ps2_video_init_once() falhou");

    if (ps2_input_init_once())
        scr_printf("input init ok\n");
    else
        scr_printf("input init falhou\n");

    ps2_menu_init();

    retro_set_environment(environ_cb);
    retro_set_video_refresh(video_cb);
    retro_set_audio_sample(audio_cb);
    retro_set_audio_sample_batch(audio_batch_cb);
    retro_set_input_poll(input_poll_cb);
    retro_set_input_state(input_state_cb);

    retro_init();

    memset(&info, 0, sizeof(info));
    retro_get_system_info(&info);

    scr_printf("core: %s %s\n",
               info.library_name ? info.library_name : "(null)",
               info.library_version ? info.library_version : "(null)");

    scr_printf("need_fullpath=%d block_extract=%d valid_ext=%s\n",
               info.need_fullpath,
               info.block_extract,
               info.valid_extensions ? info.valid_extensions : "(null)");

    rom_size = (size_t)(smw_sfc_end - smw_sfc_start);

    memset(&game, 0, sizeof(game));
    game.path = "smw.sfc";
    game.data = smw_sfc_start;
    game.size = rom_size;
    game.meta = NULL;

    scr_printf("embedded rom size: %u bytes\n", (unsigned)rom_size);

    if (!retro_load_game(&game))
        die("retro_load_game() falhou");

    memset(&av, 0, sizeof(av));
    retro_get_system_av_info(&av);
    if (av.timing.fps > 1.0)
        g_core_nominal_fps = av.timing.fps;

    scr_printf("retro_load_game() OK\n");
    scr_printf("core nominal fps: %.3f\n", g_core_nominal_fps);
    scr_printf("entrando no loop...\n");

    scr_clear();

    while (1) {
        uint32_t buttons;
        uint32_t pressed;

        ps2_input_poll();
        buttons = ps2_input_buttons();
        pressed = buttons & ~g_prev_buttons;

        if (ps2_menu_is_open()) {
            ps2_menu_handle(pressed);
            if (ps2_menu_is_open())
                ps2_menu_draw();
            g_prev_buttons = buttons;
            continue;
        }

        if (pressed & PAD_SELECT) {
            ps2_menu_open();
            ps2_menu_draw();
            g_prev_buttons = buttons;
            continue;
        }

        retro_run();
        throttle_frame_if_needed();
        g_prev_buttons = buttons;
    }

    return 0;
}
