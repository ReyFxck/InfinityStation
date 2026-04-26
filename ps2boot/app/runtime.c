#include "runtime.h"

#include "state.h"
#include "overlay.h"
#include "audio.h"
#include "video.h"

#include <libpad.h>
#include <time.h>

#include "menu.h"

#define APP_MENU_SELECT_HOLD_SECONDS 2
#define APP_MENU_SELECT_HOLD_TICKS ((clock_t)(APP_MENU_SELECT_HOLD_SECONDS * CLOCKS_PER_SEC))
#define APP_MENU_SELECT_HOLD_FALLBACK_FRAMES 120u

static clock_t g_select_hold_start = 0;
static unsigned g_select_hold_frames = 0;
static int g_select_hold_triggered = 0;

static void app_runtime_reset_select_hold(void)
{
    g_select_hold_start = 0;
    g_select_hold_frames = 0;
    g_select_hold_triggered = 0;
}

static int app_runtime_select_held_long_enough(uint32_t buttons)
{
    clock_t now;

    if (!(buttons & PAD_SELECT)) {
        app_runtime_reset_select_hold();
        return 0;
    }

    if (g_select_hold_triggered)
        return 0;

    g_select_hold_frames++;

    now = clock();

    if (now != (clock_t)-1) {
        if (g_select_hold_start == 0) {
            g_select_hold_start = now ? now : 1;
            return 0;
        }

        if ((clock_t)(now - g_select_hold_start) >= APP_MENU_SELECT_HOLD_TICKS) {
            g_select_hold_triggered = 1;
            return 1;
        }
    }

    if (g_select_hold_frames >= APP_MENU_SELECT_HOLD_FALLBACK_FRAMES) {
        g_select_hold_triggered = 1;
        return 1;
    }

    return 0;
}

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
            double nominal_fps = app_overlay_get_core_nominal_fps();


            app_state_set_mode(APP_MODE_GAME);
            app_overlay_reset_timing();
            app_overlay_set_core_nominal_fps(nominal_fps);
            ps2_audio_resume();
        }

        if (prev_buttons)
            *prev_buttons = buttons;
        return 1;
    }

    if (app_runtime_select_held_long_enough(buttons)) {
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
