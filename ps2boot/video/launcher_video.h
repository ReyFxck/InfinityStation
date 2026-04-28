#ifndef PS2_LAUNCHER_VIDEO_H
#define PS2_LAUNCHER_VIDEO_H

#include <stdint.h>

#define PS2_LAUNCHER_WIDTH 640
#define PS2_LAUNCHER_HEIGHT 448

int ps2_launcher_video_init_once(void);
void ps2_launcher_video_begin_frame(uint16_t clear_color);
void ps2_launcher_video_put_pixel(unsigned x, unsigned y, uint16_t color);
void ps2_launcher_video_end_frame(void);

/* Schedule a "glass" panel for the upcoming frame. The panel is
 * rasterized by the GS as a handful of quads (border + body + top
 * band), drawn AFTER the starfield and BEFORE the UI texture overlay.
 * Replaces the previous CPU-side draw_glass_panel() which iterated
 * over every pixel of the panel rectangle.
 *
 * Pass w == 0 (or h == 0) to indicate "no panel this frame".
 *
 * begin_frame() automatically resets the scheduled panel back to
 * "none", so callers must re-arm it every frame they want it shown. */
void ps2_launcher_video_set_panel(int x, int y, int w, int h);

#endif
