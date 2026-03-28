#include "app_boot.h"
#include "app_callbacks.h"
#include "app_game.h"
#include "app_runtime.h"
#include "app_overlay.h"

#include <kernel.h>
#include <debug.h>
#include <stdint.h>
#include <stdio.h>

#include "libretro.h"
#include "ps2_input.h"
#include "platform/ps2/ps2_audio.h"
#include "ui/launcher/launcher.h"

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

int main(int argc, char *argv[])
{
    struct retro_system_av_info av;
    int saved_launcher_x;
    int saved_launcher_y;

    (void)argc;
    (void)argv;

    printf("[MAIN] before app_boot_init\n");
    app_boot_init(die);
    printf("[MAIN] after app_boot_init\n");

    printf("[MAIN] before app_callbacks_register\n");
    app_callbacks_register();
    printf("[MAIN] after app_callbacks_register\n");

    printf("[MAIN] before retro_init\n");
    retro_init();
    printf("[MAIN] after retro_init\n");

    printf("[MAIN] before app_boot_log_core_info\n");
    app_boot_log_core_info();
    printf("[MAIN] after app_boot_log_core_info\n");

    printf("[MAIN] before app_boot_run_launcher\n");
    app_boot_run_launcher(&g_prev_buttons, &saved_launcher_x, &saved_launcher_y);
    printf("[MAIN] after app_boot_run_launcher\n");

    printf("[MAIN] before app_game_load_selected\n");
    if (!app_game_load_selected())
        die("retro_load_game() falhou");
    printf("[MAIN] after app_game_load_selected\n");

    printf("[MAIN] before app_overlay_reset_timing\n");
    app_overlay_reset_timing();
    printf("[MAIN] after app_overlay_reset_timing\n");

    printf("[MAIN] before app_boot_refresh_av_info\n");
    app_boot_refresh_av_info(&av);
    printf("[MAIN] after app_boot_refresh_av_info fps=%u sample_rate=%u\n", (unsigned)av.timing.fps, (unsigned)av.timing.sample_rate);

    if (av.timing.fps > 1.0)
        app_overlay_set_core_nominal_fps(av.timing.fps);

    printf("[MAIN] before scr_clear\n");
    scr_clear();
    printf("[MAIN] after scr_clear\n");

    while (1) {
        static int g_log_loop = 0;
        static int g_log_menu = 0;
        static int g_log_run = 0;
        uint32_t buttons;
        uint32_t pressed;

        ps2_input_poll();
        buttons = ps2_input_buttons();
        pressed = buttons & ~g_prev_buttons;

        if (g_log_loop < 5) {
            printf("[MAIN] loop %d buttons=%u pressed=%u prev=%u\n",
                   g_log_loop, (unsigned)buttons, (unsigned)pressed, (unsigned)g_prev_buttons);
            g_log_loop++;
        }

        if (app_runtime_handle_menu(buttons,
                                    pressed,
                                    &g_prev_buttons,
                                    &av,
                                    &saved_launcher_x,
                                    &saved_launcher_y,
                                    die)) {
            if (g_log_menu < 10) {
                printf("[MAIN] menu consumed loop=%d buttons=%u pressed=%u\n",
                       g_log_menu, (unsigned)buttons, (unsigned)pressed);
                g_log_menu++;
            }
            continue;
        }

        if (g_log_run < 10)
            printf("[MAIN] before retro_run %d\n", g_log_run);

        retro_run();
        ps2_audio_pump();

        if (g_log_run < 10) {
            printf("[MAIN] after retro_run %d\n", g_log_run);
            g_log_run++;
        }

        app_overlay_throttle_if_needed();
        g_prev_buttons = buttons;
    }

    return 0;
}
