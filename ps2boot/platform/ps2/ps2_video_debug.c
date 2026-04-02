#include "ps2_video_internal.h"

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
