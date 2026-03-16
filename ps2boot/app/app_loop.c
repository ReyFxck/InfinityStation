#include "app_loop.h"

#include "app_game.h"
#include "app_overlay.h"

#include <debug.h>
#include <string.h>
#include <libpad.h>

#include "ps2_input.h"
#include "ps2_menu.h"
#include "ps2_video.h"
#include "ui/launcher/launcher.h"

static int app_loop_reload_selected_game(struct retro_system_av_info *av,
                                         void (*die_fn)(const char *msg))
{
    memset(av, 0, sizeof(*av));

    if (!app_game_load_selected()) {
        if (die_fn)
            die_fn("retro_load_game() falhou");
        return 0;
    }

    retro_get_system_av_info(av);
    if (av->timing.fps > 1.0)
        app_overlay_set_core_nominal_fps(av->timing.fps);

    return 1;
}

static void app_loop_prepare_reload_state(uint32_t *prev_buttons)
{
    ps2_video_set_debug("", "", "", "");

    if (prev_buttons)
        *prev_buttons = 0;

    app_overlay_reset_timing();
}

void app_loop_run_launcher(uint32_t *prev_buttons)
{
    uint32_t prev = 0;

    if (prev_buttons)
        *prev_buttons = 0;

    launcher_init();

    while (!launcher_should_start_game()) {
        uint32_t buttons;
        uint32_t pressed;

        ps2_input_poll();
        buttons = ps2_input_buttons();
        pressed = buttons & ~prev;

        launcher_handle(pressed);
        launcher_draw();

        prev = buttons;
    }

    launcher_clear_start_request();

    if (prev_buttons)
        *prev_buttons = 0;
}

int app_loop_handle_menu(uint32_t buttons,
                         uint32_t pressed,
                         uint32_t *prev_buttons,
                         struct retro_system_av_info *av,
                         int *saved_launcher_x,
                         int *saved_launcher_y,
                         void (*die_fn)(const char *msg))
{
    if (ps2_menu_is_open()) {
        ps2_menu_handle(pressed);

        if (ps2_menu_restart_game_requested()) {
            ps2_menu_clear_restart_game_request();
            ps2_menu_close();
            retro_unload_game();
            app_game_unload_loaded();
            app_loop_prepare_reload_state(prev_buttons);
            scr_clear();
            app_loop_reload_selected_game(av, die_fn);
            scr_clear();
            return 1;
        }

        if (ps2_menu_exit_game_requested()) {
            ps2_menu_clear_exit_game_request();
            ps2_menu_close();
            retro_unload_game();
            app_game_unload_loaded();
            app_loop_prepare_reload_state(prev_buttons);

            ps2_video_get_offsets(saved_launcher_x, saved_launcher_y);
            ps2_video_set_offsets(0, 0);
            scr_clear();
            app_loop_run_launcher(prev_buttons);
            ps2_video_set_offsets(*saved_launcher_x, *saved_launcher_y);

            app_loop_reload_selected_game(av, die_fn);
            scr_clear();
            return 1;
        }

        if (ps2_menu_is_open())
            ps2_menu_draw();

        if (prev_buttons)
            *prev_buttons = buttons;

        return 1;
    }

    if (pressed & PAD_SELECT) {
        ps2_menu_open();
        ps2_menu_draw();

        if (prev_buttons)
            *prev_buttons = buttons;

        return 1;
    }

    return 0;
}
