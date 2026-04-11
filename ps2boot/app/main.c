#include "app_boot.h"
#include "app_game.h"
#include "app_runtime.h"
#include "app_overlay.h"
#include "app_state.h"
#include "app_transition.h"

#include <debug.h>
#include <kernel.h>

#include "libretro.h"
#include "ps2_input.h"
#include "ps2_audio.h"
#include "ps2_video.h"
#include <stdio.h>

#define QUIET_RUNTIME_LOGS 1

#if QUIET_RUNTIME_LOGS
#undef printf
#define printf(...) ((void)0)
#endif

__attribute__((weak)) int select_menu_actions_game_vsync_enabled(void);

static uint32_t g_prev_buttons = 0;


extern const char *app_core_prof_get_line1(void);
extern const char *app_core_prof_get_line2(void);
extern const char *app_core_prof_get_line3(void);
extern const char *app_core_prof_get_line4(void);
static unsigned g_core_prof_frames = 0;
static unsigned long long g_core_prof_run_cycles = 0;
static unsigned long long g_core_prof_post_cycles = 0;
static unsigned long long g_core_prof_total_cycles = 0;

static inline unsigned app_prof_read_count(void)
{
    unsigned value;
    __asm__ __volatile__("mfc0 %0, $9" : "=r"(value));
    return value;
}

static inline float app_prof_cycles_to_ms(unsigned long long cycles, unsigned frames)
{
    if (!frames)
        return 0.0f;

    return (float)((double)cycles / (double)frames / 294912.0);
}

static void app_prof_commit(unsigned run_cycles, unsigned post_cycles, unsigned total_cycles)
{
    char l1[48];
    char l2[48];

    g_core_prof_run_cycles += run_cycles;
    g_core_prof_post_cycles += post_cycles;
    g_core_prof_total_cycles += total_cycles;
    g_core_prof_frames++;

    if (g_core_prof_frames >= 30u) {
        snprintf(l1, sizeof(l1), "RUN %.2f | POST %.2f",
                 app_prof_cycles_to_ms(g_core_prof_run_cycles, g_core_prof_frames),
                 app_prof_cycles_to_ms(g_core_prof_post_cycles, g_core_prof_frames));

        snprintf(l2, sizeof(l2), "FRAME %.2f",
                 app_prof_cycles_to_ms(g_core_prof_total_cycles, g_core_prof_frames));

        {
            const char *core_l1 = app_core_prof_get_line1();
            const char *core_l2 = app_core_prof_get_line2();
            const char *core_l3 = app_core_prof_get_line3();
            const char *core_l4 = app_core_prof_get_line4();

            if (core_l1 && core_l1[0])
                ps2_video_set_debug(
                    core_l1,
                    (core_l2 && core_l2[0]) ? core_l2 : l2,
                    (core_l3 && core_l3[0]) ? core_l3 : "",
                    (core_l4 && core_l4[0]) ? core_l4 : ""
                );
            else
                ps2_video_set_debug(l1, l2, NULL, NULL);
        }

        g_core_prof_frames = 0;
        g_core_prof_run_cycles = 0;
        g_core_prof_post_cycles = 0;
        g_core_prof_total_cycles = 0;
    }
}

static void die(const char *msg)
{
    init_scr();
    scr_printf("\n[ERRO] %s\n", msg);
    SleepThread();
    while (1) {}
}

static void app_runtime_finish_frame(void)
{
    ps2_audio_pump();
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
        unsigned t0, t1, t2;

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
        t0 = app_prof_read_count();
        retro_run();
        t1 = app_prof_read_count();
        app_runtime_finish_frame();
        t2 = app_prof_read_count();
        app_prof_commit(t1 - t0, t2 - t1, t2 - t0);
        g_prev_buttons = buttons;
    }

    return 0;
}
