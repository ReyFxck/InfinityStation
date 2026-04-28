#include "video_internal.h"


void menu_tint_blue(void)
{
    unsigned i;

    for (i = 0; i < PS2_VIDEO_TEX_PIXELS; i++) {
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
    uint16_t r = (color >> 11) & 0x1fu;
    uint16_t g = (color >>  6) & 0x1fu;
    uint16_t b =  color        & 0x1fu;
    return (uint16_t)(0x8000u | (b << 10) | (g << 5) | r);
}
