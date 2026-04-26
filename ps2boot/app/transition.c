#include "transition.h"

#include "boot.h"
#include "callbacks.h"
#include "game.h"
#include "overlay.h"
#include "launcher.h"
#include "state.h"

#include <string.h>

#include "libretro.h"
#include "video.h"
#include "audio.h"
#include <debug.h>
#include <stdio.h>
#include "common/inf_log.h"

static int g_core_initialized = 0;

static void app_transition_refresh_av_info(struct retro_system_av_info *av)
{
    if (!av)
        return;

    memset(av, 0, sizeof(*av));
    retro_get_system_av_info(av);

    if (av->timing.fps > 1.0)
        app_overlay_set_core_nominal_fps(av->timing.fps);
}

static void app_transition_core_init_once(void)
{
    INF_LOG_DBG("app_transition_core_init_once: enter initialized=%d\n", g_core_initialized);

    if (g_core_initialized) {
        INF_LOG_DBG("app_transition_core_init_once: skip already initialized\n");
        return;
    }

    app_callbacks_register();
    INF_LOG_DBG("app_transition_core_init_once: before retro_init\n");

    retro_init();

    INF_LOG_DBG("app_transition_core_init_once: after retro_init\n");

    app_boot_log_core_info();
    g_core_initialized = 1;

    INF_LOG_DBG("app_transition_core_init_once: exit initialized=%d\n", g_core_initialized);
}

static void app_transition_core_deinit_if_needed(void)
{
    INF_LOG_DBG("app_transition_core_deinit_if_needed: enter initialized=%d\n", g_core_initialized);

    if (!g_core_initialized) {
        INF_LOG_DBG("app_transition_core_deinit_if_needed: skip not initialized\n");
        return;
    }

    INF_LOG_DBG("app_transition_core_deinit_if_needed: before retro_deinit\n");

    retro_deinit();

    INF_LOG_DBG("app_transition_core_deinit_if_needed: after retro_deinit\n");

    g_core_initialized = 0;

    INF_LOG_DBG("app_transition_core_deinit_if_needed: exit initialized=%d\n", g_core_initialized);
}

void app_transition_prepare(uint32_t *prev_buttons)
{
    ps2_video_set_debug("", "", "", "");

    if (prev_buttons)
        *prev_buttons = 0;

    app_overlay_reset_timing();
}

/*
 * Limpa a tela pelo pipeline de vídeo real do projeto.
 * Isso evita depender de scr_clear(), que é do debug console.
 */
static void app_transition_flush_black_frame(void)
{
    int i;

    ps2_video_set_debug("", "", "", "");

    for (i = 0; i < 2; i++) {
        ps2_video_launcher_begin_frame(0x0000);
        ps2_video_launcher_end_frame();
    }
}

static int app_transition_try_load_selected_game(struct retro_system_av_info *av)
{
    INF_LOG_DBG("app_transition_try_load_selected_game: enter\n");

    app_transition_core_init_once();

    INF_LOG_DBG("app_transition_try_load_selected_game: before app_game_load_selected\n");

    if (!app_game_load_selected()) {
        INF_LOG_DBG("app_transition_try_load_selected_game: app_game_load_selected FAILED\n");
        return 0;
    }

    INF_LOG_DBG("app_transition_try_load_selected_game: app_game_load_selected OK\n");

    app_transition_refresh_av_info(av);

    INF_LOG_DBG("app_transition_try_load_selected_game: exit fps=%.3f\n",
           av ? av->timing.fps : 0.0);
    return 1;
}

static int app_transition_recover_full_core_reload(struct retro_system_av_info *av,
    uint32_t *prev_buttons)
{
    INF_LOG_DBG("app_transition_recover_full_core_reload: enter\n");

    app_transition_core_deinit_if_needed();
    app_transition_prepare(prev_buttons);
    app_transition_flush_black_frame();

    if (!app_transition_try_load_selected_game(av)) {
        INF_LOG_DBG("app_transition_recover_full_core_reload: FAILED\n");
        return 0;
    }

    INF_LOG_DBG("app_transition_recover_full_core_reload: success\n");
    return 1;
}

void app_transition_load_selected_game(struct retro_system_av_info *av,
    void (*die_fn)(const char *msg))
{
    INF_LOG_DBG("app_transition_load_selected_game: enter\n");

    if (!app_transition_try_load_selected_game(av)) {
        if (die_fn)
            die_fn("Falha ao carregar a ROM selecionada");
        return;
    }

    INF_LOG_DBG("app_transition_load_selected_game: exit\n");
}

void app_transition_restart_game(struct retro_system_av_info *av,
    uint32_t *prev_buttons,
    void (*die_fn)(const char *msg))
{
    INF_LOG_DBG("app_transition_restart_game: enter initialized=%d\n", g_core_initialized);

    ps2_audio_pause();

    if (g_core_initialized) {
        INF_LOG_DBG("app_transition_restart_game: before app_game_sram_autosave\n");
        app_game_sram_autosave();
        INF_LOG_DBG("app_transition_restart_game: after app_game_sram_autosave\n");

        INF_LOG_DBG("app_transition_restart_game: before retro_unload_game\n");
        retro_unload_game();
        INF_LOG_DBG("app_transition_restart_game: after retro_unload_game\n");
    }

    INF_LOG_DBG("app_transition_restart_game: before app_game_unload_loaded\n");
    app_game_unload_loaded();
    INF_LOG_DBG("app_transition_restart_game: after app_game_unload_loaded\n");

    /* manter o core inicializado entre trocas para evitar fragmentacao */
    app_transition_prepare(prev_buttons);

    INF_LOG_DBG("app_transition_restart_game: before app_transition_try_load_selected_game\n");

    if (!app_transition_try_load_selected_game(av)) {
        INF_LOG_DBG("app_transition_restart_game: fast reload failed, trying full core reload\n");

        if (!app_transition_recover_full_core_reload(av, prev_buttons)) {
            if (die_fn)
                die_fn("Falha ao reinicializar o core e recarregar a ROM");
            return;
        }
    }

    INF_LOG_DBG("app_transition_restart_game: after app_transition_try_load_selected_game\n");

    app_overlay_reset_timing();
    if (av->timing.fps > 1.0)
        app_overlay_set_core_nominal_fps(av->timing.fps);

    ps2_audio_resume();
    app_state_set_mode(APP_MODE_GAME);

    INF_LOG_DBG("app_transition_restart_game: exit\n");
}


void app_transition_open_launcher_and_reload(struct retro_system_av_info *av,
    uint32_t *prev_buttons,
    int *saved_launcher_x,
    int *saved_launcher_y,
    void (*die_fn)(const char *msg))
{
    INF_LOG_DBG("app_transition_open_launcher_and_reload: enter initialized=%d\n", g_core_initialized);

    ps2_audio_pause();

    if (g_core_initialized) {
        INF_LOG_DBG("app_transition_open_launcher_and_reload: before app_game_sram_autosave\n");
        app_game_sram_autosave();
        INF_LOG_DBG("app_transition_open_launcher_and_reload: after app_game_sram_autosave\n");

        INF_LOG_DBG("app_transition_open_launcher_and_reload: before retro_unload_game\n");
        retro_unload_game();
        INF_LOG_DBG("app_transition_open_launcher_and_reload: after retro_unload_game\n");
    }

    INF_LOG_DBG("app_transition_open_launcher_and_reload: before app_game_unload_loaded\n");
    app_game_unload_loaded();
    INF_LOG_DBG("app_transition_open_launcher_and_reload: after app_game_unload_loaded\n");

    /* manter o core inicializado entre trocas para evitar fragmentacao */
    app_transition_prepare(prev_buttons);

    ps2_video_get_offsets(saved_launcher_x, saved_launcher_y);
    ps2_video_set_offsets(0, 0);

    app_state_set_mode(APP_MODE_LAUNCHER);

    INF_LOG_DBG("app_transition_open_launcher_and_reload: before app_launcher_run\n");
    ps2_audio_pump();
    app_launcher_run(prev_buttons);
    INF_LOG_DBG("app_transition_open_launcher_and_reload: after app_launcher_run\n");

    ps2_video_set_offsets(*saved_launcher_x, *saved_launcher_y);

    INF_LOG_DBG("app_transition_open_launcher_and_reload: before app_transition_try_load_selected_game\n");

    if (!app_transition_try_load_selected_game(av)) {
        INF_LOG_DBG("app_transition_open_launcher_and_reload: fast reload failed, trying full core reload\n");

        if (!app_transition_recover_full_core_reload(av, prev_buttons)) {
            if (die_fn)
                die_fn("Falha ao reinicializar o core e recarregar a ROM");
            return;
        }
    }

    INF_LOG_DBG("app_transition_open_launcher_and_reload: after app_transition_try_load_selected_game\n");

    app_overlay_reset_timing();
    if (av->timing.fps > 1.0)
        app_overlay_set_core_nominal_fps(av->timing.fps);

    ps2_audio_resume();
    app_state_set_mode(APP_MODE_GAME);

    INF_LOG_DBG("app_transition_open_launcher_and_reload: exit\n");
}

