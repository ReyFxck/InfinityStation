#include "app_game.h"
#include "app_launcher.h"
#include "app_runtime.h"
#include "app_overlay.h"

#include <kernel.h>
#include <sifrpc.h>
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "libretro.h"
#include "ps2_video.h"
#include "ps2_input.h"
#include "ps2_menu.h"
#include "ui/launcher/launcher.h"

#define DEBUG_OVERLAY 0

static unsigned g_frame_count = 0;
static enum retro_pixel_format g_pixel_format = RETRO_PIXEL_FORMAT_RGB565;
static uint32_t g_prev_buttons = 0;

static void die(const char *msg)
{
    scr_printf("\n[ERRO] %s\n", msg);
    SleepThread();
    while (1) {}
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

    app_overlay_update_fps();

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
}

static int16_t input_state_cb(unsigned port, unsigned device, unsigned index, unsigned id)
{
    (void)port;
    (void)device;
    (void)index;
    return ps2_input_libretro_state(id);
}

int main(int argc, char *argv[])
{
    struct retro_system_info info;
    struct retro_system_av_info av;
    int saved_launcher_x;
    int saved_launcher_y;

    (void)argc;
    (void)argv;

    SifInitRpc(0);
    init_scr();

    scr_printf("ps2snes2005 launcher boot\n");

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

    ps2_video_get_offsets(&saved_launcher_x, &saved_launcher_y);
    ps2_video_set_offsets(0, 0);
    scr_clear();
    app_launcher_run(&g_prev_buttons);
    ps2_video_set_offsets(saved_launcher_x, saved_launcher_y);

    if (!app_game_load_selected())
        die("retro_load_game() falhou");

    app_overlay_reset_timing();

    memset(&av, 0, sizeof(av));
    retro_get_system_av_info(&av);
    if (av.timing.fps > 1.0)
        app_overlay_set_core_nominal_fps(av.timing.fps);

    scr_printf("retro_load_game() OK\n");
    scr_printf("selected: %s\n", launcher_selected_label());
    scr_printf("core nominal fps: %.3f\n", app_overlay_get_core_nominal_fps());
    scr_printf("entrando no loop...\n");

    scr_clear();

    while (1) {
        uint32_t buttons;
        uint32_t pressed;

        ps2_input_poll();
        buttons = ps2_input_buttons();
        pressed = buttons & ~g_prev_buttons;

        if (app_runtime_handle_menu(buttons,
                                 pressed,
                                 &g_prev_buttons,
                                 &av,
                                 &saved_launcher_x,
                                 &saved_launcher_y,
                                 die))
            continue;

        retro_run();
        app_overlay_throttle_if_needed();
        g_prev_buttons = buttons;
    }

    return 0;
}
