#include "select_menu_font.h"
#include "video.h"
#include "select_menu_font_data.h"

static void select_menu_font_draw_char(unsigned x, unsigned y, char c, uint16_t color)
{
    int row;
    int col;

    if (c >= 'a' && c <= 'z')
        c = (char)(c - ('a' - 'A'));

    for (row = 0; row < 7; row++) {
        uint8_t bits = select_menu_font_glyph_row(c, row);
        for (col = 0; col < 5; col++) {
            if (bits & (1 << (4 - col)))
                ps2_video_menu_put_pixel(x + (unsigned)col, y + (unsigned)row, color);
        }
    }
}

void select_menu_font_draw_string(unsigned x, unsigned y, const char *s)
{
    select_menu_font_draw_string_color(x, y, s, 0xFFFF);
}

void select_menu_font_draw_string_color(unsigned x, unsigned y, const char *s, uint16_t color)
{
    unsigned i;

    if (!s)
        return;

    for (i = 0; s[i]; i++)
        select_menu_font_draw_char(x + i * 6, y, s[i], color);
}
