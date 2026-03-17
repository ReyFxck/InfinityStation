#ifndef PS2_VIDEO_INTERNAL_H
#define PS2_VIDEO_INTERNAL_H

#include "ps2_video.h"
#include "ps2_menu.h"

#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include <packet.h>
#include <dma.h>
#include <graph.h>
#include <draw.h>
#include <gs_psm.h>

#define VIDEO_WAIT_VSYNC 1

static inline int clamp_int(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

extern int g_video_ready;
extern int g_lut_ready;
extern int g_video_off_x;
extern int g_video_off_y;
extern int g_aspect_mode;

extern framebuffer_t g_frame;
extern zbuffer_t g_z;
extern texbuffer_t g_tex;
extern packet_t *g_tex_packet;
extern packet_t *g_draw_packet;

extern uint16_t g_upload[256 * 224];
extern uint16_t g_frame_base[256 * 224];
extern uint16_t g_rgb565_lut[65536];

extern char g_dbg1[48];
extern char g_dbg2[48];
extern char g_dbg3[48];
extern char g_dbg4[48];

extern uint16_t g_launcher_upload[PS2_LAUNCHER_HEIGHT][PS2_LAUNCHER_WIDTH];
extern int g_ui_target_launcher;

void ps2_video_build_lut(void);
void ps2_video_apply_display_offset(void);
void ps2_video_upload_and_draw_bound(unsigned width, unsigned height, int wait_vsync);

void dbg_draw_string_color(unsigned x, unsigned y, const char *s, uint16_t color);
void dbg_overlay(void);
void menu_tint_blue(void);
void ps2_video_menu_put_pixel_store(unsigned x, unsigned y, uint16_t color);

#endif
