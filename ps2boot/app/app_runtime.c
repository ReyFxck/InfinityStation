#include "app_runtime.h"

#include "app_state.h"
#include "platform/ps2/ps2_audio.h"

#include <libpad.h>

#include "ps2_menu.h"

int app_runtime_handle_menu(uint32_t buttons, uint32_t pressed, uint32_t *prev_buttons)
{
    if (ps2_menu_is_open()) {
        app_state_set_mode(APP_MODE_MENU);

        ps2_menu_handle(pressed);

        if (ps2_menu_restart_game_requested()) {
            ps2_menu_clear_restart_game_request();
            ps2_menu_close();
            app_state_set_mode(APP_MODE_GAME);
            app_state_request(APP_REQUEST_RESTART_GAME);

            if (prev_buttons)
                *prev_buttons = buttons;
            return 1;
        }

        if (ps2_menu_exit_game_requested()) {
            ps2_menu_clear_exit_game_request();
            ps2_menu_close();
            app_state_set_mode(APP_MODE_LAUNCHER);
            app_state_request(APP_REQUEST_OPEN_LAUNCHER);

            if (prev_buttons)
                *prev_buttons = buttons;
            return 1;
        }

        if (ps2_menu_is_open()) {
            ps2_audio_pump();
            ps2_menu_draw();
        } else {
            app_state_set_mode(APP_MODE_GAME);
            ps2_audio_resume();
        }

        if (prev_buttons)
            *prev_buttons = buttons;
        return 1;
    }

    if (pressed & PAD_SELECT) {
        ps2_audio_pause();
        ps2_menu_open();
        app_state_set_mode(APP_MODE_MENU);
        ps2_audio_pump();
        ps2_menu_draw();

        if (prev_buttons)
            *prev_buttons = buttons;
        return 1;
    }

    return 0;
}
