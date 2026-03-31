#include "select_menu_actions_internal.h"

void select_menu_actions_handle_game(uint32_t pressed)
{
    select_menu_state_t *state = select_menu_state_mut();
    int count = select_menu_game_options_count();
    int vsync_index = state->show_fps ? 2 : 1;
    int frame_index = state->show_fps ? 3 : 2;

    if (pressed & PAD_UP)
        state->game_sel = select_menu_wrap_index(state->game_sel - 1, count);

    if (pressed & PAD_DOWN)
        state->game_sel = select_menu_wrap_index(state->game_sel + 1, count);

    if (state->game_sel == 0) {
        if ((pressed & PAD_LEFT) || (pressed & PAD_RIGHT) ||
            (pressed & PAD_START) || (pressed & PAD_CROSS)) {
            state->show_fps = !state->show_fps;
            state->fps_rainbow = state->show_fps ? state->fps_rainbow : 0;
            if (state->game_sel >= select_menu_game_options_count())
                state->game_sel = select_menu_game_options_count() - 1;
        }
    } else if (state->show_fps && state->game_sel == 1) {
        if ((pressed & PAD_LEFT) || (pressed & PAD_RIGHT) ||
            (pressed & PAD_START) || (pressed & PAD_CROSS))
            state->fps_rainbow = !state->fps_rainbow;
    } else if (state->game_sel == vsync_index) {
        if ((pressed & PAD_LEFT) || (pressed & PAD_RIGHT) ||
            (pressed & PAD_START) || (pressed & PAD_CROSS))
            state->game_vsync = !state->game_vsync;
    } else if (state->game_sel == frame_index) {
        if (pressed & PAD_LEFT)
            select_menu_cycle_frame_limit(-1);
        if (pressed & PAD_RIGHT)
            select_menu_cycle_frame_limit(1);
        if ((pressed & PAD_START) || (pressed & PAD_CROSS))
            select_menu_cycle_frame_limit(1);
    }

    if (pressed & PAD_CIRCLE)
        state->page = SELECT_MENU_PAGE_MAIN;
}
