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
}

/*
 * Mede o tempo gasto em retro_run e sinaliza pro modulo de audio se o
 * frame estourou o budget (1/fps + margem). Esse sinal alimenta o
 * heuristico de underrun_likely / frameskip auto: se o EE ja' esta'
 * atras da curva, vale a pena pular o PPU do proximo frame pra evitar
 * cascading slowdown -- antes mesmo do backend audsrv ficar vazio de
 * verdade.
 *
 * Margem de 10 % em cima do budget nominal pra absorver jitter de DMA
 * e RPC IOP sem disparar skip atoa. Alternativa seria EWMA, mas o
 * libretro frameskip auto ja' faz seu proprio averaging implicito
 * (so' skipa um frame por vez), entao' threshold simples basta.
 */
static void app_runtime_measure_frame(u64 start_ticks)
{
    u64 elapsed;
    double fps;
    u64 budget_ticks;

    elapsed = GetTimerSystemTime() - start_ticks;

    fps = app_overlay_get_core_nominal_fps();
    if (fps < 1.0)
        fps = 60.0;

    /* budget = 1.10 * (kBUSCLK / fps) -- 10% de margem pro jitter. */
    budget_ticks = (u64)(((double)kBUSCLK * 1.10) / fps);

    ps2_audio_note_frame_budget_overran(elapsed > budget_ticks);
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
        {
            u64 frame_start = GetTimerSystemTime();
            retro_run();
            app_runtime_measure_frame(frame_start);
        }
        app_runtime_finish_frame();
        app_prof_commit(0, 0, 0);
        g_prev_buttons = buttons;
    }

    return 0;
}
