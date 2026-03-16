#include "browser_font.h"

#include "ps2_launcher_video.h"
#include "browser_font_9x15_data.h"
#include "browser_font_symbol.h"

#define FONTX2_HEADER_SIZE 17

static unsigned browser_font_bytes_per_row(void)
{
    return (browser_font_width() + 7u) >> 3;
}

static unsigned browser_font_bytes_per_glyph(void)
{
    return browser_font_bytes_per_row() * browser_font_height();
}

static const unsigned char *browser_font_glyph_ptr(unsigned char ch)
{
    return &BROWSER_FONT_DATA_SYMBOL[
        FONTX2_HEADER_SIZE + ((unsigned)ch * browser_font_bytes_per_glyph())
    ];
}

unsigned browser_font_width(void)
{
    return (unsigned)BROWSER_FONT_DATA_SYMBOL[14];
}

unsigned browser_font_height(void)
{
    return (unsigned)BROWSER_FONT_DATA_SYMBOL[15];
}

void browser_font_draw_char(unsigned x, unsigned y, unsigned char ch, uint16_t color)
{
    const unsigned char *glyph = browser_font_glyph_ptr(ch);
    unsigned row;
    unsigned col;
    unsigned w = browser_font_width();
    unsigned h = browser_font_height();
    unsigned bpr = browser_font_bytes_per_row();

    for (row = 0; row < h; row++) {
        const unsigned char *line = glyph + (row * bpr);

        for (col = 0; col < w; col++) {
            if (line[col >> 3] & (0x80u >> (col & 7u)))
                ps2_launcher_video_put_pixel(x + col, y + row, color);
        }
    }
}

void browser_font_draw_string(unsigned x, unsigned y, const char *s, uint16_t color)
{
    unsigned step = browser_font_width() + 1u;

    if (!s)
        return;

    while (*s) {
        browser_font_draw_char(x, y, (unsigned char)*s, color);
        x += step;
        s++;
    }
}

void browser_font_draw_string_color_scaled(unsigned x, unsigned y, const char *s, uint16_t color, unsigned scale)
{
    (void)scale;
    browser_font_draw_string(x, y, s, color);
}
