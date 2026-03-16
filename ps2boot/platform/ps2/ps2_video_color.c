#include "ps2_video_internal.h"

void ps2_video_build_lut(void)
{
    unsigned i;

    if (g_lut_ready)
        return;

    for (i = 0; i < 65536; i++) {
        uint16_t c = (uint16_t)i;
        uint16_t r = (c >> 11) & 0x1f;
        uint16_t g = (c >> 5)  & 0x3f;
        uint16_t b =  c        & 0x1f;

        g >>= 1;

        g_rgb565_lut[i] = (1u << 15) | (b << 10) | (g << 5) | r;
    }

    g_lut_ready = 1;
}

void menu_tint_blue(void)
{
    unsigned i;

    for (i = 0; i < 256u * 224u; i++) {
        uint16_t c = g_upload[i];
        uint16_t r =  c        & 0x1f;
        uint16_t g = (c >> 5)  & 0x1f;
        uint16_t b = (c >> 10) & 0x1f;

        r = (uint16_t)((r * 2) / 5);
        g = (uint16_t)((g * 2) / 5);
        b = (uint16_t)clamp_int((int)((b * 3) / 5) + 10, 0, 31);

        g_upload[i] = (1u << 15) | (b << 10) | (g << 5) | r;
    }
}

uint16_t ps2_video_convert_rgb565(uint16_t color)
{
    return g_rgb565_lut[color];
}
