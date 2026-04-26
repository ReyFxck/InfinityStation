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
    printf("[DBG] app_transition_core_init_once: enter initialized=%d\n", g_core_initialized);
    fflush(stdout);

    if (g_core_initialized) {
        printf("[DBG] app_transition_core_init_once: skip already initialized\n");
        fflush(stdout);
        return;
    }

    app_callbacks_register();
    printf("[DBG] app_transition_core_init_once: before retro_init\n");
    fflush(stdout);

    retro_init();

    printf("[DBG] app_transition_core_init_once: after retro_init\n");
    fflush(stdout);

    app_boot_log_core_info();
    g_core_initialized = 1;

    printf("[DBG] app_transition_core_init_once: exit initialized=%d\n", g_core_initialized);
    fflush(stdout);
}

static void app_transition_core_deinit_if_needed(void)
{
    printf("[DBG] app_transition_core_deinit_if_needed: enter initialized=%d\n", g_core_initialized);
    fflush(stdout);

    if (!g_core_initialized) {
        printf("[DBG] app_transition_core_deinit_if_needed: skip not initialized\n");
        fflush(stdout);
        return;
    }

    printf("[DBG] app_transition_core_deinit_if_needed: before retro_deinit\n");
    fflush(stdout);

    retro_deinit();

    printf("[DBG] app_transition_core_deinit_if_needed: after retro_deinit\n");
    fflush(stdout);

    g_core_initialized = 0;

    printf("[DBG] app_transition_core_deinit_if_needed: exit initialized=%d\n", g_core_initialized);
    fflush(stdout);
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
    printf("[DBG] app_transition_try_load_selected_game: enter\n");
    fflush(stdout);

    app_transition_core_init_once();

    printf("[DBG] app_transition_try_load_selected_game: before app_game_load_selected\n");
    fflush(stdout);

    if (!app_game_load_selected()) {
        printf("[DBG] app_transition_try_load_selected_game: app_game_load_selected FAILED\n");
        fflush(stdout);
        return 0;
    }

    printf("[DBG] app_transition_try_load_selected_game: app_game_load_selected OK\n");
    fflush(stdout);

    app_transition_refresh_av_info(av);

    printf("[DBG] app_transition_try_load_selected_game: exit fps=%.3f\n",
           av ? av->timing.fps : 0.0);
    fflush(stdout);
    return 1;
}

static int app_transition_recover_full_core_reload(struct retro_system_av_info *av,
    uint32_t *prev_buttons)
{
    printf("[DBG] app_transition_recover_full_core_reload: enter\n");
    fflush(stdout);

    app_transition_core_deinit_if_needed();
    app_transition_prepare(prev_buttons);
    app_transition_flush_black_frame();

    if (!app_transition_try_load_selected_game(av)) {
        printf("[DBG] app_transition_recover_full_core_reload: FAILED\n");
        fflush(stdout);
        return 0;
    }

    printf("[DBG] app_transition_recover_full_core_reload: success\n");
    fflush(stdout);
    return 1;
}

void app_transition_load_selected_game(struct retro_system_av_info *av,
    void (*die_fn)(const char *msg))
{
    printf("[DBG] app_transition_load_selected_game: enter\n");
    fflush(stdout);

    if (!app_transition_try_load_selected_game(av)) {
        if (die_fn)
            die_fn("Falha ao carregar a ROM selecionada");
        return;
    }

    printf("[DBG] app_transition_load_selected_game: exit\n");
    fflush(stdout);
}

void app_transition_restart_game(struct retro_system_av_info *av,
    uint32_t *prev_buttons,
    void (*die_fn)(const char *msg))
{
    printf("[DBG] app_transition_restart_game: enter initialized=%d\n", g_core_initialized);
    fflush(stdout);

    ps2_audio_pause();

    if (g_core_initialized) {
        printf("[DBG] app_transition_restart_game: before app_game_sram_autosave\n");
        fflush(stdout);
        app_game_sram_autosave();
        printf("[DBG] app_transition_restart_game: after app_game_sram_autosave\n");
        fflush(stdout);

        printf("[DBG] app_transition_restart_game: before retro_unload_game\n");
        fflush(stdout);
        retro_unload_game();
        printf("[DBG] app_transition_restart_game: after retro_unload_game\n");
        fflush(stdout);
    }

    printf("[DBG] app_transition_restart_game: before app_game_unload_loaded\n");
    fflush(stdout);
    app_game_unload_loaded();
    printf("[DBG] app_transition_restart_game: after app_game_unload_loaded\n");
    fflush(stdout);

    /* manter o core inicializado entre trocas para evitar fragmentacao */
    app_transition_prepare(prev_buttons);

    printf("[DBG] app_transition_restart_game: before app_transition_try_load_selected_game\n");
    fflush(stdout);

    if (!app_transition_try_load_selected_game(av)) {
        printf("[DBG] app_transition_restart_game: fast reload failed, trying full core reload\n");
        fflush(stdout);

        if (!app_transition_recover_full_core_reload(av, prev_buttons)) {
            if (die_fn)
                die_fn("Falha ao reinicializar o core e recarregar a ROM");
            return;
        }
    }

    printf("[DBG] app_transition_restart_game: after app_transition_try_load_selected_game\n");
    fflush(stdout);

    app_overlay_reset_timing();
    if (av->timing.fps > 1.0)
        app_overlay_set_core_nominal_fps(av->timing.fps);

    ps2_audio_resume();
    app_state_set_mode(APP_MODE_GAME);

    printf("[DBG] app_transition_restart_game: exit\n");
    fflush(stdout);
}


void app_transition_open_launcher_and_reload(struct retro_system_av_info *av,
    uint32_t *prev_buttons,
    int *saved_launcher_x,
    int *saved_launcher_y,
    void (*die_fn)(const char *msg))
{
    printf("[DBG] app_transition_open_launcher_and_reload: enter initialized=%d\n", g_core_initialized);
    fflush(stdout);

    ps2_audio_pause();

    if (g_core_initialized) {
        printf("[DBG] app_transition_open_launcher_and_reload: before app_game_sram_autosave\n");
        fflush(stdout);
        app_game_sram_autosave();
        printf("[DBG] app_transition_open_launcher_and_reload: after app_game_sram_autosave\n");
        fflush(stdout);

        printf("[DBG] app_transition_open_launcher_and_reload: before retro_unload_game\n");
        fflush(stdout);
        retro_unload_game();
        printf("[DBG] app_transition_open_launcher_and_reload: after retro_unload_game\n");
        fflush(stdout);
    }

    printf("[DBG] app_transition_open_launcher_and_reload: before app_game_unload_loaded\n");
    fflush(stdout);
    app_game_unload_loaded();
    printf("[DBG] app_transition_open_launcher_and_reload: after app_game_unload_loaded\n");
    fflush(stdout);

    /* manter o core inicializado entre trocas para evitar fragmentacao */
    app_transition_prepare(prev_buttons);

    ps2_video_get_offsets(saved_launcher_x, saved_launcher_y);
    ps2_video_set_offsets(0, 0);

    app_state_set_mode(APP_MODE_LAUNCHER);

    printf("[DBG] app_transition_open_launcher_and_reload: before app_launcher_run\n");
    fflush(stdout);
    ps2_audio_pump();
    app_launcher_run(prev_buttons);
    printf("[DBG] app_transition_open_launcher_and_reload: after app_launcher_run\n");
    fflush(stdout);

    ps2_video_set_offsets(*saved_launcher_x, *saved_launcher_y);

    printf("[DBG] app_transition_open_launcher_and_reload: before app_transition_try_load_selected_game\n");
    fflush(stdout);

    if (!app_transition_try_load_selected_game(av)) {
        printf("[DBG] app_transition_open_launcher_and_reload: fast reload failed, trying full core reload\n");
        fflush(stdout);

        if (!app_transition_recover_full_core_reload(av, prev_buttons)) {
            if (die_fn)
                die_fn("Falha ao reinicializar o core e recarregar a ROM");
            return;
        }
    }

    printf("[DBG] app_transition_open_launcher_and_reload: after app_transition_try_load_selected_game\n");
    fflush(stdout);

    app_overlay_reset_timing();
    if (av->timing.fps > 1.0)
        app_overlay_set_core_nominal_fps(av->timing.fps);

    ps2_audio_resume();
    app_state_set_mode(APP_MODE_GAME);

    printf("[DBG] app_transition_open_launcher_and_reload: exit\n");
    fflush(stdout);
}

