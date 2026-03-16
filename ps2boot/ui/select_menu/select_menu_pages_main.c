#include "select_menu_pages_internal.h"

void select_menu_pages_draw_main_page(const select_menu_state_t *state)
{
    const uint16_t gray = 0x9CF3;

    select_menu_font_draw_string_color(
        select_menu_pages_center_x_for_text("MENU"),
        SELECT_MENU_TITLE_Y,
        "MENU",
        SELECT_MENU_COLOR_TEXT
    );

    select_menu_pages_draw_header_line(34, gray);

    select_menu_pages_draw_centered_menu_item(60,  "RESUME",         state->main_sel == 0);
    select_menu_pages_draw_centered_menu_item(76,  "RESTART GAME",   state->main_sel == 1);
    select_menu_pages_draw_centered_menu_item(92,  "VIDEO SETTINGS", state->main_sel == 2);
    select_menu_pages_draw_centered_menu_item(108, "GAME OPTIONS",   state->main_sel == 3);
    select_menu_pages_draw_centered_menu_item(124, "EXIT GAME",      state->main_sel == 4);

    select_menu_pages_draw_footer_line(166, gray);
    select_menu_pages_draw_footer_actions("SELECT", "BACK", "CLOSE");
}
