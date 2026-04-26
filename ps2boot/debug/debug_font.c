#include "video_internal.h"
#include "debug_font.h"

static uint16_t *g_dbg_target = NULL;
static unsigned g_dbg_target_stride = 0;
static unsigned g_dbg_target_width = 0;
static unsigned g_dbg_target_height = 0;

static uint8_t dbg_glyph_row(char c, int row)
{
    switch (c) {
        case '0': { static const uint8_t g[7] = {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}; return g[row]; }
        case '1': { static const uint8_t g[7] = {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}; return g[row]; }
        case '2': { static const uint8_t g[7] = {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}; return g[row]; }
        case '3': { static const uint8_t g[7] = {0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E}; return g[row]; }
        case '4': { static const uint8_t g[7] = {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}; return g[row]; }
        case '5': { static const uint8_t g[7] = {0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E}; return g[row]; }
        case '6': { static const uint8_t g[7] = {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}; return g[row]; }
        case '7': { static const uint8_t g[7] = {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}; return g[row]; }
        case '8': { static const uint8_t g[7] = {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}; return g[row]; }
        case '9': { static const uint8_t g[7] = {0x0E,0x11,0x11,0x0F,0x01,0x02,0x1C}; return g[row]; }
        case 'A': { static const uint8_t g[7] = {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}; return g[row]; }
        case 'B': { static const uint8_t g[7] = {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}; return g[row]; }
        case 'C': { static const uint8_t g[7] = {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}; return g[row]; }
        case 'D': { static const uint8_t g[7] = {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}; return g[row]; }
        case 'E': { static const uint8_t g[7] = {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}; return g[row]; }
        case 'F': { static const uint8_t g[7] = {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}; return g[row]; }
        case 'G': { static const uint8_t g[7] = {0x0E,0x11,0x10,0x17,0x11,0x11,0x0E}; return g[row]; }
        case 'I': { static const uint8_t g[7] = {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}; return g[row]; }
        case 'K': { static const uint8_t g[7] = {0x11,0x12,0x14,0x18,0x14,0x12,0x11}; return g[row]; }
        case 'L': { static const uint8_t g[7] = {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}; return g[row]; }
        case 'M': { static const uint8_t g[7] = {0x11,0x1B,0x15,0x15,0x11,0x11,0x11}; return g[row]; }
        case 'N': { static const uint8_t g[7] = {0x11,0x19,0x15,0x13,0x11,0x11,0x11}; return g[row]; }
        case 'O': { static const uint8_t g[7] = {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}; return g[row]; }
        case 'P': { static const uint8_t g[7] = {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}; return g[row]; }
        case 'R': { static const uint8_t g[7] = {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}; return g[row]; }
        case 'S': { static const uint8_t g[7] = {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}; return g[row]; }
        case 'T': { static const uint8_t g[7] = {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}; return g[row]; }
        case 'U': { static const uint8_t g[7] = {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}; return g[row]; }
        case 'V': { static const uint8_t g[7] = {0x11,0x11,0x11,0x11,0x11,0x0A,0x04}; return g[row]; }
        case 'X': { static const uint8_t g[7] = {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}; return g[row]; }
        case 'Y': { static const uint8_t g[7] = {0x11,0x11,0x0A,0x04,0x04,0x04,0x04}; return g[row]; }
        case '=': { static const uint8_t g[7] = {0x00,0x1F,0x00,0x1F,0x00,0x00,0x00}; return g[row]; }
        case '-': { static const uint8_t g[7] = {0x00,0x00,0x00,0x1F,0x00,0x00,0x00}; return g[row]; }
        case '>': { static const uint8_t g[7] = {0x10,0x08,0x04,0x02,0x04,0x08,0x10}; return g[row]; }
        case ' ':
        default:
            return 0x00;
    }
}

void dbg_set_target(uint16_t *buffer, unsigned stride, unsigned width, unsigned height)
{
    if (!buffer || stride == 0 || width == 0 || height == 0) {
        g_dbg_target = NULL;
        g_dbg_target_stride = 0;
        g_dbg_target_width = 0;
        g_dbg_target_height = 0;
        return;
    }

    g_dbg_target = buffer;
    g_dbg_target_stride = stride;
    g_dbg_target_width = width;
    g_dbg_target_height = height;
}

void dbg_reset_target(void)
{
    g_dbg_target = NULL;
    g_dbg_target_stride = 0;
    g_dbg_target_width = 0;
    g_dbg_target_height = 0;
}

static uint16_t *dbg_target_buffer(void)
{
    return g_dbg_target ? g_dbg_target : g_upload;
}

static unsigned dbg_target_stride(void)
{
    return g_dbg_target ? g_dbg_target_stride : PS2_VIDEO_TEX_WIDTH;
}

unsigned dbg_target_width(void)
{
    return g_dbg_target ? g_dbg_target_width : 256u;
}

unsigned dbg_target_height(void)
{
    return g_dbg_target ? g_dbg_target_height : 224u;
}

static unsigned dbg_scale_x(void)
{
    unsigned w = dbg_target_width();
    unsigned sx = (w + 255u) / 256u;

    if (sx < 1u)
        sx = 1u;
    if (sx > 2u)
        sx = 2u;
    return sx;
}

static unsigned dbg_scale_y(void)
{
    unsigned h = dbg_target_height();
    unsigned sy = (h + 223u) / 224u;

    if (sy < 1u)
        sy = 1u;
    if (sy > 2u)
        sy = 2u;
    return sy;
}

static void dbg_put_pixel(unsigned x, unsigned y, uint16_t color)
{
    uint16_t *dst = dbg_target_buffer();
    unsigned stride = dbg_target_stride();

    if (x >= dbg_target_width() || y >= dbg_target_height())
        return;

    dst[y * stride + x] = color;
}

static void dbg_draw_char(unsigned x, unsigned y, char c, uint16_t color)
{
    unsigned sx = dbg_scale_x();
    unsigned sy = dbg_scale_y();
    int row, col;

    for (row = 0; row < 7; row++) {
        uint8_t bits = dbg_glyph_row(c, row);
        for (col = 0; col < 5; col++) {
            if (bits & (1 << (4 - col))) {
                unsigned dx, dy;
                for (dy = 0; dy < sy; dy++) {
                    for (dx = 0; dx < sx; dx++) {
                        dbg_put_pixel(
                            x + (unsigned)col * sx + dx,
                            y + (unsigned)row * sy + dy,
                            color
                        );
                    }
                }
            }
        }
    }
}

void dbg_draw_string_color(unsigned x, unsigned y, const char *s, uint16_t color)
{
    unsigned i;
    unsigned adv = 5u * dbg_scale_x();

    for (i = 0; s[i]; i++) {
        dbg_draw_char(x + i * adv, y, s[i], color);
    }
}
