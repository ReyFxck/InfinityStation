#include "app_boot.h"
#include "app_callbacks.h"
#include "app_game.h"
#include "app_runtime.h"
#include "app_overlay.h"

#include <kernel.h>
#include <debug.h>
#include <stdint.h>

#include "libretro.h"
#include "ps2_input.h"
#include "ui/launcher/launcher.h"

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

    app_boot_init(die);
    app_callbacks_register();

    retro_init();
    app_boot_log_core_info();

    app_boot_run_launcher(&g_prev_buttons, &saved_launcher_x, &saved_launcher_y);

    if (!app_game_load_selected())
        die("retro_load_game() falhou");

    app_overlay_reset_timing();

    app_boot_refresh_av_info(&av);
    if (av.timing.fps > 1.0)
        app_overlay_set_core_nominal_fps(av.timing.fps);


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
