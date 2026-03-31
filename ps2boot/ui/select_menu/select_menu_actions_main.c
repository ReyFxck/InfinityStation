#include "select_menu_actions_internal.h"

void select_menu_actions_handle_main(uint32_t pressed)
{
    select_menu_state_t *state = select_menu_state_mut();

    /* preserva teu fix do CIRCLE fechando no menu principal */
    if (pressed & PAD_CIRCLE) {
        state->open = 0;
        state->page = SELECT_MENU_PAGE_MAIN;
        return;
    }

    if (pressed & PAD_UP)
        state->main_sel = select_menu_wrap_index(state->main_sel - 1, 5);

    if (pressed & PAD_DOWN)
        state->main_sel = select_menu_wrap_index(state->main_sel + 1, 5);

    if (pressed & (PAD_START | PAD_CROSS)) {
        if (state->main_sel == 0) {
            state->open = 0;
        } else if (state->main_sel == 1) {
            state->pending_action = SELECT_MENU_ACTION_RESTART;
            state->open = 0;
            state->page = SELECT_MENU_PAGE_MAIN;
        } else if (state->main_sel == 2) {
            state->page = SELECT_MENU_PAGE_VIDEO;
            state->video_sel = 0;
        } else if (state->main_sel == 3) {
            state->page = SELECT_MENU_PAGE_GAME_OPTIONS;
            state->game_sel = 0;
        } else {
            state->pending_action = SELECT_MENU_ACTION_OPEN_LAUNCHER;
            state->open = 0;
            state->page = SELECT_MENU_PAGE_MAIN;
        }
    }
}
