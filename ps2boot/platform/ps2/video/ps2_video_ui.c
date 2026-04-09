#include "ps2_video_internal.h"

void ps2_video_ui_set_menu_target(void)
{
    g_ui_target_launcher = 0;
}

void ps2_video_ui_put_pixel(unsigned x, unsigned y, uint16_t color)
{
    ps2_video_menu_put_pixel_store(x, y, color);
}

uint16_t ps2_video_ui_get_pixel(unsigned x, unsigned y)
{
    if (g_ui_target_launcher) {
        if (x >= PS2_LAUNCHER_WIDTH || y >= PS2_LAUNCHER_HEIGHT)
            return 0;
        return g_launcher_upload[y][x];
    }

    if (x >= PS2_VIDEO_TEX_WIDTH || y >= PS2_VIDEO_TEX_HEIGHT)
        return 0;

    return g_upload[y * PS2_VIDEO_TEX_WIDTH + x];
}
