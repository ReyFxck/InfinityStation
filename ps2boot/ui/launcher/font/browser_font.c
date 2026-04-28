#include "browser_font.h"

#include <string.h>

#include "launcher_video.h"
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

/* Bold style is implemented as a horizontal doublestrike: every ON
 * pixel of the bitmap glyph is also painted one pixel to the right.
 * This effectively doubles the stroke thickness of every glyph (a
 * 1px-wide stroke becomes 2px wide) without needing a separate bold
 * font asset. The step (advance per character) is bumped by 1 to
 * preserve inter-character spacing. */
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
            if (line[col >> 3] & (0x80u >> (col & 7u))) {
                ps2_launcher_video_put_pixel(x + col, y + row, color);
                ps2_launcher_video_put_pixel(x + col + 1u, y + row, color);
            }
        }
    }
}

void browser_font_draw_string(unsigned x, unsigned y, const char *s, uint16_t color)
{
    /* +2 = original 1px gap + 1px occupied by the doublestrike. */
    unsigned step = browser_font_width() + 2u;

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
    unsigned ch_w;
    unsigned ch_h;
    unsigned step;

    if (!s)
        return;

    if (scale == 0)
        scale = 1;

    ch_w = browser_font_width();
    ch_h = browser_font_height();
    /* +scale extra step per char compensates for the doublestrike
     * (which extends each ON pixel one src-column to the right,
     * i.e. one full scale-block in dest coords). */
    step = (ch_w + 1u) * scale + scale;

    while (*s) {
        const unsigned char *glyph = browser_font_glyph_ptr((unsigned char)*s);
        unsigned row;
        unsigned col;
        unsigned sx;
        unsigned sy;
        unsigned bpr = browser_font_bytes_per_row();

        for (row = 0; row < ch_h; row++) {
            const unsigned char *line = glyph + (row * bpr);

            for (col = 0; col < ch_w; col++) {
                if (!(line[col >> 3] & (0x80u >> (col & 7u))))
                    continue;

                for (sy = 0; sy < scale; sy++) {
                    for (sx = 0; sx < scale; sx++) {
                        ps2_launcher_video_put_pixel(
                            x + col * scale + sx,
                            y + row * scale + sy,
                            color
                        );
                        /* doublestrike: paint the same fragment one
                         * scale-block to the right, thickening every
                         * vertical stroke by one src-column. */
                        ps2_launcher_video_put_pixel(
                            x + col * scale + sx + scale,
                            y + row * scale + sy,
                            color
                        );
                    }
                }
            }
        }

        x += step;
        s++;
    }
}

void browser_font_draw_char_sized(unsigned x, unsigned y, unsigned char ch, uint16_t color, unsigned out_w, unsigned out_h)
{
    const unsigned char *glyph = browser_font_glyph_ptr(ch);
    unsigned src_w = browser_font_width();
    unsigned src_h = browser_font_height();
    unsigned bpr = browser_font_bytes_per_row();
    unsigned dx;
    unsigned dy;

    if (out_w == 0 || out_h == 0)
        return;

    for (dy = 0; dy < out_h; dy++) {
        unsigned sy = (dy * src_h) / out_h;
        const unsigned char *line = glyph + (sy * bpr);

        for (dx = 0; dx < out_w; dx++) {
            unsigned sx = (dx * src_w) / out_w;

            if (line[sx >> 3] & (0x80u >> (sx & 7u))) {
                ps2_launcher_video_put_pixel(x + dx, y + dy, color);
                /* doublestrike: also paint the next dest column for
                 * a horizontal bold effect. */
                ps2_launcher_video_put_pixel(x + dx + 1u, y + dy, color);
            }
        }
    }
}

void browser_font_draw_string_color_sized(unsigned x, unsigned y, const char *s, uint16_t color, unsigned out_w, unsigned out_h)
{
    unsigned step;

    if (!s || out_w == 0 || out_h == 0)
        return;

    /* +3 = original 2px gap + 1px occupied by the doublestrike. */
    step = out_w + 3u;

    while (*s) {
        browser_font_draw_char_sized(x, y, (unsigned char)*s, color, out_w, out_h);
        x += step;
        s++;
    }
}
