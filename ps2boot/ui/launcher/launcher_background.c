#include "launcher_background.h"

#include <graph.h>
#include <stdint.h>

#include "ps2_launcher_video.h"

extern const unsigned int launcher_bg_ntsc_width;
extern const unsigned int launcher_bg_ntsc_height;
extern const uint16_t launcher_bg_ntsc_rgb565[];

extern const unsigned int launcher_bg_pal_width;
extern const unsigned int launcher_bg_pal_height;
extern const uint16_t launcher_bg_pal_rgb565[];

void launcher_background_draw(void)
{
    const uint16_t *pixels;
    unsigned width;
    unsigned height;
    unsigned draw_w;
    unsigned draw_h;
    unsigned x;
    unsigned y;

    if (graph_get_region() == GRAPH_MODE_PAL) {
        pixels = launcher_bg_pal_rgb565;
        width = launcher_bg_pal_width;
        height = launcher_bg_pal_height;
    } else {
        pixels = launcher_bg_ntsc_rgb565;
        width = launcher_bg_ntsc_width;
        height = launcher_bg_ntsc_height;
    }

    draw_w = width;
    draw_h = height;

    if (draw_w > PS2_LAUNCHER_WIDTH)
        draw_w = PS2_LAUNCHER_WIDTH;
    if (draw_h > PS2_LAUNCHER_HEIGHT)
        draw_h = PS2_LAUNCHER_HEIGHT;

    for (y = 0; y < draw_h; y++) {
        const uint16_t *row = &pixels[y * width];
        for (x = 0; x < draw_w; x++) {
            ps2_launcher_video_put_pixel(x, y, row[x]);
        }
    }
}
