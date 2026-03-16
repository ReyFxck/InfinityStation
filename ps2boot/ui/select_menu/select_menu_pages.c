#include "select_menu_pages_internal.h"

#include <string.h>

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

unsigned select_menu_pages_center_x_for_text(const char *text)
{
    size_t len = text ? strlen(text) : 0;
    unsigned width = (unsigned)(len * 6);

    if (width >= 256)
        return 0;

    return (256 - width) / 2;
}

unsigned select_menu_pages_text_width_px(const char *text)
{
    size_t len = text ? strlen(text) : 0;
    return (unsigned)(len * 6);
}

void select_menu_pages_draw_header_line(unsigned y, uint16_t color)
{
    unsigned x;
    for (x = 28; x < 228; x++)
        ps2_video_menu_put_pixel(x, y, color);
}

void select_menu_pages_draw_footer_line(unsigned y, uint16_t color)
{
    unsigned x;
    for (x = 20; x < 236; x++)
        ps2_video_menu_put_pixel(x, y, color);
}

void select_menu_pages_draw_icon_cross(unsigned x, unsigned y)
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

    draw_icon_bitmap(x, y, icon, 7, 0x001F);
}

void select_menu_pages_draw_icon_circle(unsigned x, unsigned y)
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

    draw_icon_bitmap(x, y, icon, 9, 0xF800);
}

void select_menu_pages_draw_icon_select(unsigned x, unsigned y)
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

    draw_icon_bitmap(x, y, icon, 7, 0x6B2D);
}

void select_menu_pages_draw_centered_menu_item(unsigned y, const char *label, int selected)
{
    unsigned text_x = select_menu_pages_center_x_for_text(label);

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

void select_menu_pages_draw_footer_actions(const char *label0, const char *label1, const char *label2)
{
    const unsigned icon0_w = 7;
    const unsigned icon1_w = 9;
    const unsigned icon2_w = 11;
    const unsigned icon_gap = 5;
    const unsigned group_gap = 18;

    unsigned group0_w = icon0_w + icon_gap + select_menu_pages_text_width_px(label0);
    unsigned group1_w = icon1_w + icon_gap + select_menu_pages_text_width_px(label1);
    unsigned group2_w = icon2_w + icon_gap + select_menu_pages_text_width_px(label2);
    unsigned total_w = group0_w + group_gap + group1_w + group_gap + group2_w;
    unsigned start_x = (256 - total_w) / 2;
    unsigned y_icon = 172;
    unsigned y_text = 172;

    unsigned x0 = start_x;
    unsigned x1 = x0 + group0_w + group_gap;
    unsigned x2 = x1 + group1_w + group_gap;

    select_menu_pages_draw_icon_cross(x0, y_icon);
    select_menu_font_draw_string_color(x0 + icon0_w + icon_gap, y_text, label0, SELECT_MENU_COLOR_TEXT);

    select_menu_pages_draw_icon_circle(x1, y_icon - 1);
    select_menu_font_draw_string_color(x1 + icon1_w + icon_gap, y_text, label1, SELECT_MENU_COLOR_TEXT);

    select_menu_pages_draw_icon_select(x2, y_icon);
    select_menu_font_draw_string_color(x2 + icon2_w + icon_gap, y_text, label2, SELECT_MENU_COLOR_TEXT);
}

void select_menu_pages_draw_frame_limit_rail(unsigned y, int current_mode, int selected)
{
    static const int xs[4] = { 52, 96, 140, 188 };
    static const char *labels[4] = { "AUTO", "50", "60", "OFF" };
    uint16_t base_color = 0x5294;
    uint16_t active_color = selected ? SELECT_MENU_COLOR_HIGHLIGHT : SELECT_MENU_COLOR_TEXT;
    unsigned i;
    unsigned x;
    unsigned rail_y = y + 10;

    for (x = (unsigned)(xs[0] + 8); x < (unsigned)(xs[3] + 16); x++)
        ps2_video_menu_put_pixel(x, rail_y, base_color);

    for (i = 0; i < 4; i++) {
        unsigned w = select_menu_pages_text_width_px(labels[i]);
        unsigned tx = (xs[i] > (int)(w / 2)) ? (unsigned)(xs[i] - (int)(w / 2)) : 0;
        uint16_t c = ((int)i == current_mode) ? active_color : base_color;

        select_menu_font_draw_string_color(tx, y, labels[i], c);

        ps2_video_menu_put_pixel((unsigned)xs[i], rail_y, c);
        ps2_video_menu_put_pixel((unsigned)xs[i] + 1, rail_y, c);
        ps2_video_menu_put_pixel((unsigned)xs[i], rail_y + 1, c);
        ps2_video_menu_put_pixel((unsigned)xs[i] + 1, rail_y + 1, c);
    }
}

void select_menu_pages_draw(const select_menu_state_t *state)
{
    if (!state)
        return;

    if (state->page == SELECT_MENU_PAGE_MAIN)
        select_menu_pages_draw_main_page(state);
    else if (state->page == SELECT_MENU_PAGE_VIDEO)
        select_menu_pages_draw_video_page(state);
    else if (state->page == SELECT_MENU_PAGE_VIDEO_DISPLAY)
        select_menu_pages_draw_display_page();
    else if (state->page == SELECT_MENU_PAGE_VIDEO_ASPECT)
        select_menu_pages_draw_aspect_page(state);
    else
        select_menu_pages_draw_game_page(state);
}
