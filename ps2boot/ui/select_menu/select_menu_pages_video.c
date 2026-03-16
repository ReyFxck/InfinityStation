#include "select_menu_pages_internal.h"

#include <stdio.h>

void select_menu_pages_draw_video_page(const select_menu_state_t *state)
{
    const uint16_t gray = 0x9CF3;

    select_menu_font_draw_string_color(
        select_menu_pages_center_x_for_text("VIDEO"),
        SELECT_MENU_TITLE_Y,
        "VIDEO",
        SELECT_MENU_COLOR_TEXT
    );

    select_menu_pages_draw_header_line(34, gray);

    select_menu_pages_draw_centered_menu_item(76,  "DISPLAY POSITION", state->video_sel == 0);
    select_menu_pages_draw_centered_menu_item(92,  "ASPECT RATIO",     state->video_sel == 1);
    select_menu_pages_draw_centered_menu_item(108, "BACK",             state->video_sel == 2);

    select_menu_pages_draw_footer_line(166, gray);
    select_menu_pages_draw_footer_actions("OPEN", "BACK", "CLOSE");
}

void select_menu_pages_draw_display_page(void)
{
    char buf_x[40];
    char buf_y[40];
    int x;
    int y;

    ps2_video_get_offsets(&x, &y);

    snprintf(buf_x, sizeof(buf_x), "X = %d", x);
    snprintf(buf_y, sizeof(buf_y), "Y = %d", y);

    select_menu_font_draw_string_color(SELECT_MENU_DISPLAY_TITLE_X, SELECT_MENU_TITLE_Y, "DISPLAY POSITION", SELECT_MENU_COLOR_TEXT);
    select_menu_font_draw_string_color(SELECT_MENU_DISPLAY_INFO_X,  SELECT_MENU_DISPLAY_INFO_Y, "D-PAD MOVES OUTPUT", SELECT_MENU_COLOR_TEXT);
    select_menu_font_draw_string_color(SELECT_MENU_DISPLAY_X_X,     SELECT_MENU_DISPLAY_X_Y, buf_x, SELECT_MENU_COLOR_HIGHLIGHT);
    select_menu_font_draw_string_color(SELECT_MENU_DISPLAY_Y_X,     SELECT_MENU_DISPLAY_Y_Y, buf_y, SELECT_MENU_COLOR_HIGHLIGHT);
    select_menu_font_draw_string_color(SELECT_MENU_DISPLAY_HINT_X,  SELECT_MENU_DISPLAY_HINT_Y, "CROSS START CIRCLE = BACK", SELECT_MENU_COLOR_TEXT);
    select_menu_font_draw_string_color(8, SELECT_MENU_HINT_BOTTOM_Y, "SELECT CLOSE", SELECT_MENU_COLOR_TEXT);
}

void select_menu_pages_draw_aspect_page(const select_menu_state_t *state)
{
    select_menu_font_draw_string_color(SELECT_MENU_ASPECT_TITLE_X, SELECT_MENU_TITLE_Y, "ASPECT RATIO", SELECT_MENU_COLOR_TEXT);

    select_menu_font_draw_string_color(
        SELECT_MENU_ASPECT_ITEM0_X, SELECT_MENU_ASPECT_ITEM0_Y,
        state->aspect_sel == 0 ? "> 4:3" : "  4:3",
        state->aspect_sel == 0 ? SELECT_MENU_COLOR_HIGHLIGHT : SELECT_MENU_COLOR_TEXT
    );

    select_menu_font_draw_string_color(
        SELECT_MENU_ASPECT_ITEM1_X, SELECT_MENU_ASPECT_ITEM1_Y,
        state->aspect_sel == 1 ? "> 16:9" : "  16:9",
        state->aspect_sel == 1 ? SELECT_MENU_COLOR_HIGHLIGHT : SELECT_MENU_COLOR_TEXT
    );

    select_menu_font_draw_string_color(
        SELECT_MENU_ASPECT_ITEM2_X, SELECT_MENU_ASPECT_ITEM2_Y,
        state->aspect_sel == 2 ? "> FULL SCREEN" : "  FULL SCREEN",
        state->aspect_sel == 2 ? SELECT_MENU_COLOR_HIGHLIGHT : SELECT_MENU_COLOR_TEXT
    );

    select_menu_font_draw_string_color(
        SELECT_MENU_ASPECT_ITEM3_X, SELECT_MENU_ASPECT_ITEM3_Y,
        state->aspect_sel == 3 ? "> PIXEL PERFECT" : "  PIXEL PERFECT",
        state->aspect_sel == 3 ? SELECT_MENU_COLOR_HIGHLIGHT : SELECT_MENU_COLOR_TEXT
    );

    select_menu_font_draw_string_color(
        SELECT_MENU_ASPECT_ITEM4_X, SELECT_MENU_ASPECT_ITEM4_Y,
        state->aspect_sel == 4 ? "> BACK" : "  BACK",
        state->aspect_sel == 4 ? SELECT_MENU_COLOR_HIGHLIGHT : SELECT_MENU_COLOR_TEXT
    );

    select_menu_font_draw_string_color(SELECT_MENU_ASPECT_HINT_X, SELECT_MENU_ASPECT_HINT_Y, "CROSS START = APPLY", SELECT_MENU_COLOR_TEXT);
    select_menu_font_draw_string_color(8, SELECT_MENU_HINT_BOTTOM_Y, "SELECT CLOSE", SELECT_MENU_COLOR_TEXT);
}
