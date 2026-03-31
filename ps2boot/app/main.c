#include "app_boot.h"
#include "app_callbacks.h"
#include "app_game.h"
#include "app_launcher.h"
#include "app_runtime.h"
#include "app_overlay.h"
#include "app_state.h"

#include <debug.h>
#include <kernel.h>
#include <stdio.h>
#include <string.h>

#include "libretro.h"
#include "ps2_input.h"
#include "platform/ps2/ps2_audio.h"
#include "ps2_video.h"

/* QUIET_RUNTIME_LOGS_BEGIN */
#define QUIET_RUNTIME_LOGS 1
#if QUIET_RUNTIME_LOGS
#undef printf
#define printf(...) ((void)0)
#endif
/* QUIET_RUNTIME_LOGS_END */

static uint32_t g_prev_buttons = 0;

static void die(const char *msg)
{
    init_scr();
    scr_printf("\n[ERRO] %s\n", msg);
    SleepThread();
    while (1) {}
}

static void app_main_refresh_av_info(struct retro_system_av_info *av)
{
    memset(av, 0, sizeof(*av));
    retro_get_system_av_info(av);

    if (av->timing.fps > 1.0)
        app_overlay_set_core_nominal_fps(av->timing.fps);
}

static void app_main_prepare_transition(void)
{
    ps2_video_set_debug("", "", "", "");
    g_prev_buttons = 0;
    app_overlay_reset_timing();
}

static void app_main_load_selected_game(struct retro_system_av_info *av)
{
    if (!app_game_load_selected())
        die("retro_load_game() falhou");

    app_main_refresh_av_info(av);
}

static void app_main_restart_game(struct retro_system_av_info *av)
{
    retro_unload_game();
    app_game_unload_loaded();
    app_main_prepare_transition();
    scr_clear();
    app_main_load_selected_game(av);
    scr_clear();
    app_state_set_mode(APP_MODE_GAME);
}

static void app_main_open_launcher_and_reload(struct retro_system_av_info *av,
                                              int *saved_launcher_x,
                                              int *saved_launcher_y)
{
    retro_unload_game();
    app_game_unload_loaded();
    app_main_prepare_transition();

    ps2_video_get_offsets(saved_launcher_x, saved_launcher_y);
    ps2_video_set_offsets(0, 0);

    scr_clear();
    app_state_set_mode(APP_MODE_LAUNCHER);
    app_launcher_run(&g_prev_buttons);
    ps2_video_set_offsets(*saved_launcher_x, *saved_launcher_y);

    app_main_load_selected_game(av);
    scr_clear();
    app_state_set_mode(APP_MODE_GAME);
}

int main(int argc, char *argv[])
{
    struct retro_system_av_info av;
    int saved_launcher_x;
    int saved_launcher_y;

    (void)argc;
    (void)argv;

    app_state_init();

    app_boot_init(die);
    app_callbacks_register();
    retro_init();
    app_boot_log_core_info();

    app_state_set_mode(APP_MODE_LAUNCHER);
    app_boot_run_launcher(&g_prev_buttons, &saved_launcher_x, &saved_launcher_y);

    app_main_load_selected_game(&av);
    app_overlay_reset_timing();
    app_boot_refresh_av_info(&av);

    if (av.timing.fps > 1.0)
        app_overlay_set_core_nominal_fps(av.timing.fps);

    scr_clear();
    app_state_set_mode(APP_MODE_GAME);

    while (1) {
        uint32_t buttons;
        uint32_t pressed;
        int menu_consumed;
        app_request_t req;

        ps2_input_poll();
        buttons = ps2_input_buttons();
        pressed = buttons & ~g_prev_buttons;

        menu_consumed = app_runtime_handle_menu(buttons, pressed, &g_prev_buttons);

        req = app_state_take_request();
        if (req == APP_REQUEST_RESTART_GAME) {
            app_main_restart_game(&av);
            continue;
        } else if (req == APP_REQUEST_OPEN_LAUNCHER) {
            app_main_open_launcher_and_reload(&av, &saved_launcher_x, &saved_launcher_y);
            continue;
        }

        if (menu_consumed)
            continue;

        retro_run();
        ps2_audio_pump();
        app_overlay_throttle_if_needed();
        g_prev_buttons = buttons;
    }

    return 0;
}
