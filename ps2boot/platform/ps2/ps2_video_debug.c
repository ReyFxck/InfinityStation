#include "ps2_video_internal.h"

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

static void dbg_put_pixel(unsigned x, unsigned y, uint16_t color)
{
    if (x >= 256 || y >= 224)
        return;

    g_upload[y * 256 + x] = color;
}

static void dbg_draw_char(unsigned x, unsigned y, char c, uint16_t color)
{
    int row, col;

    for (row = 0; row < 7; row++) {
        uint8_t bits = dbg_glyph_row(c, row);
        for (col = 0; col < 5; col++) {
            if (bits & (1 << (4 - col)))
                dbg_put_pixel(x + (unsigned)col, y + (unsigned)row, color);
        }
    }
}

void dbg_draw_string_color(unsigned x, unsigned y, const char *s, uint16_t color)
{
    unsigned i;
    uint16_t shadow = 0x8000;

    for (i = 0; s[i]; i++) {
        dbg_draw_char(x + i * 6 + 1, y + 1, s[i], shadow);
        dbg_draw_char(x + i * 6,     y,     s[i], color);
    }
}

static void dbg_draw_string(unsigned x, unsigned y, const char *s)
{
    dbg_draw_string_color(x, y, s, 0xFFFF);
}

static uint16_t dbg_rainbow_color(unsigned phase)
{
    unsigned region = (phase >> 8) % 6;
    unsigned t = phase & 0xFF;
    unsigned r = 0;
    unsigned g = 0;
    unsigned b = 0;

    switch (region) {
        case 0: r = 255;     g = t;       b = 0;         break;
        case 1: r = 255 - t; g = 255;     b = 0;         break;
        case 2: r = 0;       g = 255;     b = t;         break;
        case 3: r = 0;       g = 255 - t; b = 255;       break;
        case 4: r = t;       g = 0;       b = 255;       break;
        default:
            r = 255;         g = 0;       b = 255 - t;   break;
    }

    return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

void ps2_video_set_debug(const char *line1, const char *line2, const char *line3, const char *line4)
{
    if (line1) { strncpy(g_dbg1, line1, sizeof(g_dbg1) - 1); g_dbg1[sizeof(g_dbg1) - 1] = 0; }
    if (line2) { strncpy(g_dbg2, line2, sizeof(g_dbg2) - 1); g_dbg2[sizeof(g_dbg2) - 1] = 0; }
    if (line3) { strncpy(g_dbg3, line3, sizeof(g_dbg3) - 1); g_dbg3[sizeof(g_dbg3) - 1] = 0; }
    if (line4) { strncpy(g_dbg4, line4, sizeof(g_dbg4) - 1); g_dbg4[sizeof(g_dbg4) - 1] = 0; }
}

void dbg_overlay(void)
{
    static unsigned rainbow_phase = 0;
    uint16_t c = 0xFFFF;

    if (!g_dbg1[0] && !g_dbg2[0] && !g_dbg3[0] && !g_dbg4[0])
        return;

    if (ps2_menu_fps_rainbow_enabled()) {
        rainbow_phase = (rainbow_phase + 24) % 1536;
        c = dbg_rainbow_color(rainbow_phase);

        dbg_draw_string_color(2,  2, g_dbg1, c);
        dbg_draw_string_color(2, 10, g_dbg2, c);
        dbg_draw_string_color(2, 18, g_dbg3, c);
        dbg_draw_string_color(2, 26, g_dbg4, c);
        return;
    }

    dbg_draw_string(2,  2, g_dbg1);
    dbg_draw_string(2, 10, g_dbg2);
    dbg_draw_string(2, 18, g_dbg3);
    dbg_draw_string(2, 26, g_dbg4);
}
