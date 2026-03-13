#include "select_menu_pages.h"

#include "../../ps2_video.h"
#include <stdio.h>
#include "select_menu_theme.h"
#include "font/select_menu_font.h"

static void draw_main_page(const select_menu_state_t *state)
{
    select_menu_font_draw_string_color(115, SELECT_MENU_TITLE_Y, "MENU", SELECT_MENU_COLOR_TEXT);

    select_menu_font_draw_string_color(
        SELECT_MENU_MAIN_RESUME_X, SELECT_MENU_MAIN_RESUME_Y,
        state->main_sel == 0 ? "> RESUME" : "  RESUME",
        state->main_sel == 0 ? SELECT_MENU_COLOR_HIGHLIGHT : SELECT_MENU_COLOR_TEXT
    );

    select_menu_font_draw_string_color(
        SELECT_MENU_MAIN_VIDEO_X, SELECT_MENU_MAIN_VIDEO_Y,
        state->main_sel == 1 ? "> VIDEO" : "  VIDEO",
        state->main_sel == 1 ? SELECT_MENU_COLOR_HIGHLIGHT : SELECT_MENU_COLOR_TEXT
    );

    select_menu_font_draw_string_color(8, SELECT_MENU_HINT_BOTTOM_Y, "SELECT CLOSE", SELECT_MENU_COLOR_TEXT);
}

static void draw_video_page(const select_menu_state_t *state)
{
    select_menu_font_draw_string_color(SELECT_MENU_VIDEO_TITLE_X, SELECT_MENU_TITLE_Y, "VIDEO", SELECT_MENU_COLOR_TEXT);

    select_menu_font_draw_string_color(
        SELECT_MENU_VIDEO_ITEM0_X, SELECT_MENU_VIDEO_ITEM0_Y,
        state->video_sel == 0 ? "> DISPLAY POSITION" : "  DISPLAY POSITION",
        state->video_sel == 0 ? SELECT_MENU_COLOR_HIGHLIGHT : SELECT_MENU_COLOR_TEXT
    );

    select_menu_font_draw_string_color(
        SELECT_MENU_VIDEO_ITEM1_X, SELECT_MENU_VIDEO_ITEM1_Y,
        state->video_sel == 1 ? "> ASPECT RATIO" : "  ASPECT RATIO",
        state->video_sel == 1 ? SELECT_MENU_COLOR_HIGHLIGHT : SELECT_MENU_COLOR_TEXT
    );

    select_menu_font_draw_string_color(
        SELECT_MENU_VIDEO_ITEM2_X, SELECT_MENU_VIDEO_ITEM2_Y,
        state->video_sel == 2 ? "> BACK" : "  BACK",
        state->video_sel == 2 ? SELECT_MENU_COLOR_HIGHLIGHT : SELECT_MENU_COLOR_TEXT
    );

    select_menu_font_draw_string_color(8, SELECT_MENU_HINT_UPPER_Y, "CROSS = OPEN", SELECT_MENU_COLOR_TEXT);
    select_menu_font_draw_string_color(8, SELECT_MENU_HINT_BOTTOM_Y, "SELECT CLOSE", SELECT_MENU_COLOR_TEXT);
}

static void draw_display_page(void)
{
    char buf_x[40];
    char buf_y[40];
    int x;
    int y;

    ps2_video_get_offsets(&x, &y);

    snprintf(buf_x, sizeof(buf_x), "X = %d", x);
    snprintf(buf_y, sizeof(buf_y), "Y = %d", y);

    select_menu_font_draw_string_color(SELECT_MENU_DISPLAY_TITLE_X, SELECT_MENU_TITLE_Y, "DISPLAY POSITION", SELECT_MENU_COLOR_TEXT);
    select_menu_font_draw_string_color(SELECT_MENU_DISPLAY_INFO_X, SELECT_MENU_DISPLAY_INFO_Y, "D-PAD MOVES OUTPUT", SELECT_MENU_COLOR_TEXT);
    select_menu_font_draw_string_color(SELECT_MENU_DISPLAY_X_X, SELECT_MENU_DISPLAY_X_Y, buf_x, SELECT_MENU_COLOR_HIGHLIGHT);
    select_menu_font_draw_string_color(SELECT_MENU_DISPLAY_Y_X, SELECT_MENU_DISPLAY_Y_Y, buf_y, SELECT_MENU_COLOR_HIGHLIGHT);
    select_menu_font_draw_string_color(SELECT_MENU_DISPLAY_HINT_X, SELECT_MENU_DISPLAY_HINT_Y, "CROSS START CIRCLE = BACK", SELECT_MENU_COLOR_TEXT);
    select_menu_font_draw_string_color(8, SELECT_MENU_HINT_BOTTOM_Y, "SELECT CLOSE", SELECT_MENU_COLOR_TEXT);
}

static void draw_aspect_page(const select_menu_state_t *state)
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

void select_menu_pages_draw(const select_menu_state_t *state)
{
    if (!state)
        return;

    if (state->page == SELECT_MENU_PAGE_MAIN)
        draw_main_page(state);
    else if (state->page == SELECT_MENU_PAGE_VIDEO)
        draw_video_page(state);
    else if (state->page == SELECT_MENU_PAGE_VIDEO_DISPLAY)
        draw_display_page();
    else
        draw_aspect_page(state);
}
