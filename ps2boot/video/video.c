#include "video_internal.h"

int g_video_ready = 0;
int g_video_off_x = 0;
int g_video_off_y = 0;
int g_aspect_mode = PS2_ASPECT_FULL;

/*
 * Vblanks pendentes pra esperar fora do retro_run.
 *
 * O caminho antigo chamava graph_wait_vsync() la' DENTRO do
 * ps2_video_packets_upload_and_draw, que e' chamado durante o video_cb
 * do libretro, que esta' DENTRO do retro_run. Resultado: nao da pra
 * medir o tempo de "trabalho real" do retro_run sem incluir o wait
 * de vsync (~16.6 ms), o que quebra qualquer detector preditivo de
 * frame budget.
 *
 * Agora ps2_video_present_rgb565 ACUMULA aqui o numero de vblanks que
 * o frame quer esperar e retorna sem bloquear. O main loop chama
 * ps2_video_finish_frame() DEPOIS de retro_run, e e' la' que o
 * graph_wait_vsync acontece. Assim a janela de vsync fica fora da
 * medicao de retro_run.
 */
unsigned g_video_pending_vblanks = 0;

framebuffer_t g_frame;
zbuffer_t g_z;
texbuffer_t g_tex;
packet_t *g_tex_packets[PS2_VIDEO_TEX_SLOTS] = {0};
packet_t *g_draw_packets[PS2_VIDEO_TEX_SLOTS] = {0};

uint16_t g_upload[PS2_VIDEO_TEX_PIXELS] __attribute__((aligned(64)));
uint16_t g_upload_256[PS2_VIDEO_UPLOAD_256_PIXELS] __attribute__((aligned(64)));

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

void ps2_video_finish_frame(void)
{
    unsigned waits;

    if (!g_video_ready)
        return;

    waits = g_video_pending_vblanks;
    g_video_pending_vblanks = 0;

    while (waits > 0) {
        graph_wait_vsync();
        waits--;
    }
}
