#include "video_internal.h"

void ps2_video_launcher_begin_frame(uint16_t clear_color)
{
    unsigned y;
    unsigned x;

    g_ui_target_launcher = 1;

    for (y = 0; y < PS2_LAUNCHER_HEIGHT; y++) {
        for (x = 0; x < PS2_LAUNCHER_WIDTH; x++) {
            g_launcher_upload[y][x] = clear_color;
        }
    }
}

void ps2_video_launcher_end_frame(void)
{
    ps2_video_present_ui_fixed_rgb565(
        g_launcher_upload,
        PS2_LAUNCHER_WIDTH,
        PS2_LAUNCHER_HEIGHT,
        PS2_LAUNCHER_WIDTH * sizeof(uint16_t)
    );
    g_ui_target_launcher = 0;
}
