#include "app_transition.h"

#include "app_boot.h"
#include "app_callbacks.h"
#include "app_game.h"
#include "app_overlay.h"
#include "app_launcher.h"
#include "app_state.h"

#include <string.h>

#include "libretro.h"
#include "ps2_video.h"
#include "ps2_audio.h"

#include <debug.h>

static int g_core_initialized = 0;

static void app_transition_refresh_av_info(struct retro_system_av_info *av)
{
    memset(av, 0, sizeof(*av));
    retro_get_system_av_info(av);
    if (av->timing.fps > 1.0)
        app_overlay_set_core_nominal_fps(av->timing.fps);
}

static void app_transition_core_init_once(void)
{
    if (g_core_initialized)
        return;

    app_callbacks_register();
    retro_init();
    app_boot_log_core_info();
    g_core_initialized = 1;
}

static void app_transition_core_deinit_if_needed(void)
{
    if (!g_core_initialized)
        return;

    retro_deinit();
    g_core_initialized = 0;
}

void app_transition_prepare(uint32_t *prev_buttons)
{
    ps2_video_set_debug("", "", "", "");
    if (prev_buttons)
        *prev_buttons = 0;
    app_overlay_reset_timing();
}

void app_transition_load_selected_game(struct retro_system_av_info *av,
                                       void (*die_fn)(const char *msg))
{
    app_transition_core_init_once();

    if (!app_game_load_selected()) {
        if (die_fn)
            die_fn("retro_load_game() falhou");
        return;
    }

    app_transition_refresh_av_info(av);
}

void app_transition_restart_game(struct retro_system_av_info *av,
                                 uint32_t *prev_buttons,
                                 void (*die_fn)(const char *msg))
{
    ps2_audio_pause();
    ps2_audio_pause();

    if (g_core_initialized)
        retro_unload_game();

    app_game_unload_loaded();
    app_transition_prepare(prev_buttons);
    scr_clear();
    app_transition_load_selected_game(av, die_fn);
    ps2_audio_resume();
    scr_clear();
    app_state_set_mode(APP_MODE_GAME);
}

void app_transition_open_launcher_and_reload(struct retro_system_av_info *av,
                                             uint32_t *prev_buttons,
                                             int *saved_launcher_x,
                                             int *saved_launcher_y,
                                             void (*die_fn)(const char *msg))
{
    if (g_core_initialized)
        retro_unload_game();

    app_game_unload_loaded();
    app_transition_core_deinit_if_needed();
    app_transition_prepare(prev_buttons);

    ps2_video_get_offsets(saved_launcher_x, saved_launcher_y);
    ps2_video_set_offsets(0, 0);
    scr_clear();

    app_state_set_mode(APP_MODE_LAUNCHER);
    ps2_audio_pump();
    app_launcher_run(prev_buttons);

    ps2_video_set_offsets(*saved_launcher_x, *saved_launcher_y);
    app_transition_load_selected_game(av, die_fn);
    ps2_audio_resume();
    scr_clear();
    app_state_set_mode(APP_MODE_GAME);
}
