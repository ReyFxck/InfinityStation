#include "select_menu_pages_internal.h"

void select_menu_pages_draw(const select_menu_state_t *state,
                            const select_menu_view_state_t *view_state)
{
    if (!state)
        return;

    if (state->page == SELECT_MENU_PAGE_MAIN) {
        select_menu_pages_draw_main_page(state);
    } else if (state->page == SELECT_MENU_PAGE_VIDEO) {
        select_menu_pages_draw_video_page(state);
    } else if (state->page == SELECT_MENU_PAGE_VIDEO_DISPLAY) {
        select_menu_pages_draw_display_page(view_state);
    } else if (state->page == SELECT_MENU_PAGE_VIDEO_ASPECT) {
        select_menu_pages_draw_aspect_page(state);
    } else {
        select_menu_pages_draw_game_page(state);
    }
}
