#include "select_menu_pages.h"

#include <stdio.h>
#include <string.h>
#include "../../ps2_video.h"
#include "select_menu_theme.h"
#include "font/select_menu_font.h"

static const char *frame_limit_label(int mode)
{
    if (mode == SELECT_MENU_FRAME_LIMIT_50)
        return "50";
    if (mode == SELECT_MENU_FRAME_LIMIT_60)
        return "60";
    if (mode == SELECT_MENU_FRAME_LIMIT_OFF)
        return "OFF";
    return "AUTO";
}

static unsigned center_x_for_text(const char *text)
{
    size_t len = text ? strlen(text) : 0;
    unsigned width = (unsigned)(len * 6);

    if (width >= 256)
        return 0;

    return (256 - width) / 2;
}

static unsigned text_width_px(const char *text)
{
    size_t len = text ? strlen(text) : 0;
    return (unsigned)(len * 6);
}

static void draw_header_line(unsigned y, uint16_t color)
{
    unsigned x;
    for (x = 28; x < 228; x++)
        ps2_video_menu_put_pixel(x, y, color);
}

static void draw_footer_line(unsigned y, uint16_t color)
{
    unsigned x;
    for (x = 20; x < 236; x++)
        ps2_video_menu_put_pixel(x, y, color);
}

static void draw_icon_bitmap(unsigned x, unsigned y, const char **rows, unsigned h, uint16_t color)
{
    unsigned yy;
    unsigned xx;

    for (yy = 0; yy < h; yy++) {
        unsigned w = (unsigned)strlen(rows[yy]);
        for (xx = 0; xx < w; xx++) {
            if (rows[yy][xx] == '#')
                ps2_video_menu_put_pixel(x + xx, y + yy, color);
        }
    }
}

static void draw_icon_cross(unsigned x, unsigned y)
{
    static const char *icon[] = {
        "#.....#",
        ".#...#.",
        "..#.#..",
        "...#...",
        "..#.#..",
        ".#...#.",
        "#.....#"
    };

    draw_icon_bitmap(x, y, icon, 7, 0xF800);
}

static void draw_icon_circle(unsigned x, unsigned y)
{
    static const char *icon[] = {
        "...###...",
        "..#...#..",
        ".#.....#.",
        "#.......#",
        "#.......#",
        "#.......#",
        ".#.....#.",
        "..#...#..",
        "...###..."
    };

    draw_icon_bitmap(x, y, icon, 9, 0x001F);
}

static void draw_icon_select(unsigned x, unsigned y)
{
    static const char *icon[] = {
        ".#########.",
        "#.........#",
        "#.........#",
        "#.........#",
        "#.........#",
        "#.........#",
        ".#########."
    };

    draw_icon_bitmap(x, y, icon, 7, 0x8CB4);
}

static void draw_centered_menu_item(unsigned y, const char *label, int selected)
{
    unsigned text_x = center_x_for_text(label);

    if (selected) {
        unsigned arrow_x = (text_x >= 12) ? (text_x - 12) : 0;
        select_menu_font_draw_string_color(
            arrow_x,
            y,
            ">",
            SELECT_MENU_COLOR_HIGHLIGHT
        );
    }

    select_menu_font_draw_string_color(
        text_x,
        y,
        label,
        selected ? SELECT_MENU_COLOR_HIGHLIGHT : SELECT_MENU_COLOR_TEXT
    );
}

static void draw_main_footer_actions(void)
{
    const char *label0 = "SELECT";
    const char *label1 = "BACK";
    const char *label2 = "CLOSE";

    const unsigned icon0_w = 7;
    const unsigned icon1_w = 9;
    const unsigned icon2_w = 11;
    const unsigned icon_gap = 5;
    const unsigned group_gap = 18;

    unsigned group0_w = icon0_w + icon_gap + text_width_px(label0);
    unsigned group1_w = icon1_w + icon_gap + text_width_px(label1);
    unsigned group2_w = icon2_w + icon_gap + text_width_px(label2);
    unsigned total_w = group0_w + group_gap + group1_w + group_gap + group2_w;
    unsigned start_x = (256 - total_w) / 2;
    unsigned y_icon = 172;
    unsigned y_text = 172;

    unsigned x0 = start_x;
    unsigned x1 = x0 + group0_w + group_gap;
    unsigned x2 = x1 + group1_w + group_gap;

    draw_icon_cross(x0, y_icon);
    select_menu_font_draw_string_color(x0 + icon0_w + icon_gap, y_text, label0, SELECT_MENU_COLOR_TEXT);

    draw_icon_circle(x1, y_icon - 1);
    select_menu_font_draw_string_color(x1 + icon1_w + icon_gap, y_text, label1, SELECT_MENU_COLOR_TEXT);

    draw_icon_select(x2, y_icon);
    select_menu_font_draw_string_color(x2 + icon2_w + icon_gap, y_text, label2, SELECT_MENU_COLOR_TEXT);
}

static void draw_main_page(const select_menu_state_t *state)
{
    const uint16_t gray = 0x742F;

    select_menu_font_draw_string_color(117, SELECT_MENU_TITLE_Y, "MENU", SELECT_MENU_COLOR_TEXT);
    draw_header_line(34, gray);

    draw_centered_menu_item(60,  "RESUME",         state->main_sel == 0);
    draw_centered_menu_item(76,  "RESTART GAME",   state->main_sel == 1);
    draw_centered_menu_item(92,  "VIDEO SETTINGS", state->main_sel == 2);
    draw_centered_menu_item(108, "GAME OPTIONS",   state->main_sel == 3);
    draw_centered_menu_item(124, "EXIT GAME",      state->main_sel == 4);

    draw_footer_line(166, gray);
    draw_main_footer_actions();
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

static void draw_game_options_page(const select_menu_state_t *state)
{
    char line0[40];
    char line1[40];

    snprintf(line0, sizeof(line0), "%s SHOW FPS: %s",
             state->game_sel == 0 ? ">" : " ",
             state->show_fps ? "ON" : "OFF");

    snprintf(line1, sizeof(line1), "%s FRAME LIMIT: %s",
             state->game_sel == 1 ? ">" : " ",
             frame_limit_label(state->frame_limit));

    select_menu_font_draw_string_color(70, SELECT_MENU_TITLE_Y, "GAME OPTIONS", SELECT_MENU_COLOR_TEXT);

    select_menu_font_draw_string_color(
        56, 84, line0,
        state->game_sel == 0 ? SELECT_MENU_COLOR_HIGHLIGHT : SELECT_MENU_COLOR_TEXT
    );

    select_menu_font_draw_string_color(
        44, 100, line1,
        state->game_sel == 1 ? SELECT_MENU_COLOR_HIGHLIGHT : SELECT_MENU_COLOR_TEXT
    );

    select_menu_font_draw_string_color(
        114, 118,
        state->game_sel == 2 ? "> BACK" : "  BACK",
        state->game_sel == 2 ? SELECT_MENU_COLOR_HIGHLIGHT : SELECT_MENU_COLOR_TEXT
    );

    select_menu_font_draw_string_color(18, 170, "LEFT RIGHT OR CROSS TO CHANGE", SELECT_MENU_COLOR_TEXT);
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
    else if (state->page == SELECT_MENU_PAGE_VIDEO_ASPECT)
        draw_aspect_page(state);
    else
        draw_game_options_page(state);
}
