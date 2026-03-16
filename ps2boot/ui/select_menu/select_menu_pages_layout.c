#include "select_menu_pages_internal.h"

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
