#include "select_menu_pages_internal.h"

#include <string.h>

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
