#include "ps2_video.h"

#include <string.h>

#include <packet.h>
#include <dma.h>
#include <graph.h>
#include <draw.h>
#include <gs_psm.h>

static int g_video_ready = 0;
static framebuffer_t g_frame;
static zbuffer_t g_z;
static texbuffer_t g_tex;
static packet_t *g_tex_packet = 0;
static packet_t *g_draw_packet = 0;

static uint16_t g_upload[256 * 256] __attribute__((aligned(64)));

static uint16_t rgb565_to_ps2_16(uint16_t c)
{
    uint16_t r = (c >> 11) & 0x1f;
    uint16_t g = (c >> 5)  & 0x3f;
    uint16_t b =  c        & 0x1f;

    g >>= 1;

    return (1u << 15) | (b << 10) | (g << 5) | r;
}

int ps2_video_init_once(void)
{
    packet_t *packet;
    qword_t *q;
    lod_t lod;
    clutbuffer_t clut;

    if (g_video_ready)
        return 1;

    dma_channel_initialize(DMA_CHANNEL_GIF, 0, 0);
    dma_channel_fast_waits(DMA_CHANNEL_GIF);

    g_frame.width = 640;
    g_frame.height = 448;
    g_frame.mask = 0;
    g_frame.psm = GS_PSM_32;
    g_frame.address = graph_vram_allocate(
        g_frame.width, g_frame.height, g_frame.psm, GRAPH_ALIGN_PAGE
    );

    memset(&g_z, 0, sizeof(g_z));
    g_z.enable = DRAW_DISABLE;

    g_tex.width = 256;
    g_tex.psm = GS_PSM_16;
    g_tex.address = graph_vram_allocate(256, 256, GS_PSM_16, GRAPH_ALIGN_BLOCK);
    g_tex.info.width = draw_log2(256);
    g_tex.info.height = draw_log2(256);
    g_tex.info.components = TEXTURE_COMPONENTS_RGB;
    g_tex.info.function = TEXTURE_FUNCTION_DECAL;

    graph_initialize(
        g_frame.address, g_frame.width, g_frame.height, g_frame.psm, 0, 0
    );

    packet = packet_init(32, PACKET_NORMAL);
    if (!packet)
        return 0;

    q = packet->data;
    q = draw_setup_environment(q, 0, &g_frame, &g_z);
    q = draw_finish(q);

    dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
    dma_wait_fast();
    packet_free(packet);

    memset(&lod, 0, sizeof(lod));
    lod.calculation = LOD_USE_K;
    lod.max_level = 0;
    lod.mag_filter = LOD_MAG_NEAREST;
    lod.min_filter = LOD_MIN_NEAREST;
    lod.l = 0;
    lod.k = 0.0f;

    memset(&clut, 0, sizeof(clut));
    clut.storage_mode = CLUT_STORAGE_MODE1;
    clut.load_method = CLUT_NO_LOAD;

    packet = packet_init(16, PACKET_NORMAL);
    if (!packet)
        return 0;

    q = packet->data;
    q = draw_texture_sampling(q, 0, &lod);
    q = draw_texturebuffer(q, 0, &g_tex, &clut);
    q = draw_finish(q);

    dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
    dma_wait_fast();
    packet_free(packet);

    g_tex_packet = packet_init(128, PACKET_NORMAL);
    g_draw_packet = packet_init(64, PACKET_NORMAL);

    if (!g_tex_packet || !g_draw_packet)
        return 0;

    g_video_ready = 1;
    return 1;
}

void ps2_video_present_rgb565(const void *data, unsigned width, unsigned height, size_t pitch)
{
    const uint8_t *src = (const uint8_t *)data;
    qword_t *q;
    texrect_t rect;
    unsigned y;

    if (!g_video_ready || !data || width == 0 || height == 0)
        return;

    if (width > 256)
        width = 256;
    if (height > 256)
        height = 256;

    for (y = 0; y < height; y++) {
        const uint16_t *line = (const uint16_t *)(src + (y * pitch));
        unsigned x;

        for (x = 0; x < width; x++) {
            g_upload[y * 256 + x] = rgb565_to_ps2_16(line[x]);
        }
    }

    dma_wait_fast();

    q = g_tex_packet->data;
    q = draw_texture_transfer(q, g_upload, 256, 256, GS_PSM_16, g_tex.address, g_tex.width);
    q = draw_texture_flush(q);
    dma_channel_send_chain(DMA_CHANNEL_GIF, g_tex_packet->data, q - g_tex_packet->data, 0, 0);
    dma_wait_fast();

    memset(&rect, 0, sizeof(rect));

    rect.v0.x = 64.0f;
    rect.v0.y = 0.0f;
    rect.v0.z = 0;

    rect.v1.x = 576.0f;
    rect.v1.y = 448.0f;
    rect.v1.z = 0;

    rect.t0.u = 0.0f;
    rect.t0.v = 0.0f;
    rect.t1.u = (float)width - 1.0f;
    rect.t1.v = (float)height - 1.0f;

    rect.color.r = 0x80;
    rect.color.g = 0x80;
    rect.color.b = 0x80;
    rect.color.a = 0x80;
    rect.color.q = 1.0f;

    q = g_draw_packet->data;
    q = draw_clear(q, 0, 0.0f, 0.0f, (float)g_frame.width, (float)g_frame.height, 0, 0, 0);
    q = draw_rect_textured(q, 0, &rect);
    q = draw_finish(q);

    dma_channel_send_normal(DMA_CHANNEL_GIF, g_draw_packet->data, q - g_draw_packet->data, 0, 0);
    draw_wait_finish();
    graph_wait_vsync();
}
