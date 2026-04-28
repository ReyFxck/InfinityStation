#include "boot.h"
#include "game.h"
#include "runtime.h"
#include "overlay.h"
#include "state.h"
#include "transition.h"

#include <debug.h>
#include <kernel.h>
#include <timer.h>

#include "libretro.h"
#include "input.h"
#include "audio.h"
#include "video.h"
#include "common/inf_log.h"

__attribute__((weak)) int select_menu_actions_game_vsync_enabled(void);

static uint32_t g_prev_buttons = 0;


static void app_prof_commit(unsigned run_cycles, unsigned post_cycles, unsigned total_cycles)
{
    (void)run_cycles;
    (void)post_cycles;
    (void)total_cycles;
}

static void die(const char *msg)
{
    u64 deadline;

    init_scr();
    scr_printf("\n[ERRO] %s\n", msg);
    scr_printf("\nVoltando ao browser do PS2 em 5s...\n");

    deadline = GetTimerSystemTime() + (u64)kBUSCLK * 5;
    while (GetTimerSystemTime() < deadline) { }

    LoadExecPS2("rom0:OSDSYS", 0, NULL);
}

static void app_runtime_finish_frame(void)
{
    ps2_audio_pump();
    /*
     * Drena o vsync_wait pendente DEPOIS de retro_run + audio_pump.
     * Isso e' o que move a espera de vblank pra fora da janela do
     * core, deixando retro_run medir so' trabalho real do EE/GS/audio.
     * Idempotente: e' no-op se nada esta' pendente (ex.: vsync OFF
     * + frame_limit OFF, que nem sequer agendou wait).
     */
    ps2_video_finish_frame();
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

    app_state_set_mode(APP_MODE_LAUNCHER);
    app_boot_run_launcher(&g_prev_buttons, &saved_launcher_x, &saved_launcher_y);

    app_transition_load_selected_game(&av, die);
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
            app_transition_restart_game(&av, &g_prev_buttons, die);
            continue;
        } else if (req == APP_REQUEST_OPEN_LAUNCHER) {
            app_transition_open_launcher_and_reload(
                &av,
                &g_prev_buttons,
                &saved_launcher_x,
                &saved_launcher_y,
                die
            );
            continue;
        }

        if (menu_consumed)
            continue;
        retro_run();
        app_runtime_finish_frame();
        app_prof_commit(0, 0, 0);
        g_prev_buttons = buttons;
    }

    return 0;
}
