#include "ps2_video_internal.h"

int g_video_ready = 0;
int g_lut_ready = 0;
int g_video_off_x = 0;
int g_video_off_y = 0;
int g_aspect_mode = PS2_ASPECT_4_3;

framebuffer_t g_frame;
zbuffer_t g_z;
texbuffer_t g_tex;
packet_t *g_tex_packet = 0;
packet_t *g_draw_packet = 0;

uint16_t g_upload[PS2_VIDEO_TEX_PIXELS] __attribute__((aligned(64)));
uint16_t g_frame_base[PS2_VIDEO_TEX_PIXELS] __attribute__((aligned(64)));
uint16_t g_upload_256[PS2_VIDEO_UPLOAD_256_PIXELS] __attribute__((aligned(64)));
uint16_t g_rgb565_lut[65536] __attribute__((aligned(64)));

char g_dbg1[48] = "";
char g_dbg2[48] = "";
char g_dbg3[48] = "";
char g_dbg4[48] = "";

uint16_t g_launcher_upload[PS2_LAUNCHER_HEIGHT][PS2_LAUNCHER_WIDTH];
int g_ui_target_launcher = 0;

void ps2_video_set_offsets(int x, int y)
{
    g_video_off_x = clamp_int(x, -96, 96);
    g_video_off_y = clamp_int(y, -64, 64);

    if (g_video_ready)
        ps2_video_apply_display_offset();
}

void ps2_video_get_offsets(int *x, int *y)
{
    if (x) *x = g_video_off_x;
    if (y) *y = g_video_off_y;
}

void ps2_video_set_aspect(int mode)
{
    if (mode < PS2_ASPECT_4_3)
        mode = PS2_ASPECT_4_3;
    if (mode > PS2_ASPECT_PIXEL)
        mode = PS2_ASPECT_PIXEL;

    g_aspect_mode = mode;
}

int ps2_video_get_aspect(void)
{
    return g_aspect_mode;
}

uint32_t ps2_video_frame_address(void)
{
    return g_frame.address;
}

unsigned ps2_video_frame_width(void)
{
    return g_frame.width;
}

unsigned ps2_video_frame_height(void)
{
    return g_frame.height;
}
