#ifndef PS2_VIDEO_INTERNAL_H
#define PS2_VIDEO_INTERNAL_H

#include "video.h"
#include "debug_font.h"
#include "menu.h"

#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include <packet.h>
#include <dma.h>
#include <graph.h>
#include <draw.h>
#include <gs_psm.h>

#define VIDEO_WAIT_VSYNC 1

#define PS2_VIDEO_TEX_SLOTS   4u
#define PS2_VIDEO_TEX_WIDTH   512u
#define PS2_VIDEO_TEX_HEIGHT  256u
#define PS2_VIDEO_TEX_PIXELS  (PS2_VIDEO_TEX_WIDTH * PS2_VIDEO_TEX_HEIGHT)
#define PS2_VIDEO_UPLOAD_256_WIDTH 256u
#define PS2_VIDEO_UPLOAD_256_PIXELS (PS2_VIDEO_UPLOAD_256_WIDTH * PS2_VIDEO_TEX_HEIGHT)

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
extern packet_t *g_tex_packets[PS2_VIDEO_TEX_SLOTS];
extern packet_t *g_draw_packets[PS2_VIDEO_TEX_SLOTS];

extern uint16_t g_upload[PS2_VIDEO_TEX_PIXELS];
extern uint16_t g_upload_256[PS2_VIDEO_UPLOAD_256_PIXELS];
extern uint16_t g_rgb565_lut[65536];

extern char g_dbg1[48];
extern char g_dbg2[48];
extern char g_dbg3[48];
extern char g_dbg4[48];

extern uint16_t g_launcher_upload[PS2_LAUNCHER_HEIGHT][PS2_LAUNCHER_WIDTH];
extern int g_ui_target_launcher;

int select_menu_actions_game_vsync_enabled(void);

void ps2_video_build_lut(void);
void ps2_video_apply_display_offset(void);
void ps2_video_present_ui_fixed_rgb565(const void *data, unsigned width, unsigned height, size_t pitch);
void ps2_video_upload_and_draw_bound(unsigned width, unsigned height, int wait_vsync);

/* video_prof.c */
static inline unsigned ps2_video_prof_read_count(void)
{
    unsigned value;
    __asm__ __volatile__("mfc0 %0, $9" : "=r"(value));
    return value;
}

static inline unsigned ps2_video_prof_delta(unsigned t1, unsigned t0)
{
    return (unsigned)(t1 - t0);
}

void ps2_video_prof_commit(
    unsigned cvt_cycles,
    unsigned ovl_cycles,
    unsigned backend_cycles,
    unsigned total_cycles,
    unsigned width,
    unsigned height
);

/* video_cache.c */
int  ps2_video_cache_can_reuse_256(const uint16_t *src, unsigned width, unsigned height);
void ps2_video_cache_store_256(const uint16_t *src, unsigned width, unsigned height);
void ps2_video_cache_invalidate(void);

/* video_packets.c */
int  ps2_video_packets_init(void);
void ps2_video_packets_reset(int hard);
int  ps2_video_packets_last_upload_valid(void);
void ps2_video_packets_hard_reset_redraw_textures(const uint16_t *src);
void ps2_video_packets_upload_and_draw(
    const uint16_t *upload,
    unsigned upload_width,
    unsigned upload_height,
    unsigned width,
    unsigned height,
    unsigned wait_vblanks
);
void ps2_video_packets_redraw_last(unsigned width, unsigned height, unsigned wait_vblanks);

void dbg_overlay(void);
void menu_tint_blue(void);
void ps2_video_menu_put_pixel_store(unsigned x, unsigned y, uint16_t color);

#endif
