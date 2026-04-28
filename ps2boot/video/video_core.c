#include "video_internal.h"
#include <stdio.h>
#include <kernel.h>
#include "common/inf_log.h"

void ps2_video_apply_display_offset(void)
{
    graph_set_screen(g_video_off_x, g_video_off_y, g_frame.width, g_frame.height);
}

int ps2_video_init_once(void)
{
    packet_t *packet;
    qword_t *q;

    if (g_video_ready)
        return 1;

    dma_channel_initialize(DMA_CHANNEL_GIF, 0, 0);
    dma_channel_fast_waits(DMA_CHANNEL_GIF);

    g_frame.width = 640;
    g_frame.height = 448;
    g_frame.mask = 0;
    g_frame.psm = GS_PSM_32;
    g_frame.address = graph_vram_allocate(
        g_frame.width,
        g_frame.height,
        g_frame.psm,
        GRAPH_ALIGN_PAGE
    );

    memset(&g_z, 0, sizeof(g_z));
    g_z.enable = DRAW_DISABLE;

    /* INF: graph_set_mode explicit
     * Em vez de graph_initialize() (que usa defaults do SDK e
     * costuma cair em INTERLACED + FIELD = flicker em fontes 224p
     * como SNES), configuramos a região por graph_get_region() e
     * forçamos GRAPH_MODE_FRAME (mesmo conteúdo nos dois subcampos).
     */
    {
        int region = graph_get_region();
        int psm_mode = (region == GRAPH_MODE_PAL) ? GRAPH_MODE_PAL : GRAPH_MODE_NTSC;
        graph_set_mode(GRAPH_MODE_INTERLACED, psm_mode, GRAPH_MODE_FIELD, GRAPH_DISABLE); /* P17: era FRAME, cortava metade */
        graph_set_screen(0, 0, g_frame.width, g_frame.height);
        graph_set_bgcolor(0, 0, 0);
        graph_set_framebuffer_filtered(g_frame.address, g_frame.width, g_frame.psm, 0, 0);
        graph_enable_output();
    }
    ps2_video_apply_display_offset();

    packet = packet_init(32, PACKET_NORMAL);
    if (!packet)
        return 0;

    q = packet->data;
    q = draw_setup_environment(q, 0, &g_frame, &g_z);
    q = draw_clear(q, 0, 0.0f, 0.0f, (float)g_frame.width, (float)g_frame.height, 0, 0, 0);
    q = draw_finish(q);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
    dma_wait_fast();
    packet_free(packet);

    if (!ps2_video_packets_init())
        return 0;

    g_video_ready = 1;
    return 1;
}

void ps2_video_soft_reset(void)
{
    packet_t *packet;
    qword_t *q;

    if (!g_video_ready)
        return;

    INF_LOG_DBG("ps2_video_soft_reset: enter\n");

    draw_wait_finish();
    dma_wait_fast();

    memset(g_upload, 0, sizeof(g_upload));
    memset(g_upload_256, 0, sizeof(g_upload_256));
    memset(g_launcher_upload, 0, sizeof(g_launcher_upload));

    ps2_video_packets_reset(0);
    ps2_video_cache_invalidate();

    SyncDCache((void *)g_upload,
               (void *)((unsigned char *)g_upload + sizeof(g_upload)));

    packet = packet_init(32, PACKET_NORMAL);
    if (!packet) {
        INF_LOG_DBG("ps2_video_soft_reset: packet_init failed\n");
        return;
    }

    q = packet->data;
    q = draw_setup_environment(q, 0, &g_frame, &g_z);
    q = draw_clear(q, 0, 0.0f, 0.0f, (float)g_frame.width, (float)g_frame.height, 0, 0, 0);
    q = draw_finish(q);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
    dma_wait_fast();
    packet_free(packet);

    INF_LOG_DBG("ps2_video_soft_reset: exit\n");
}

void ps2_video_hard_reset(void)
{
    packet_t *packet;
    qword_t *q;

    if (!g_video_ready)
        return;

    draw_wait_finish();
    dma_wait_fast();

    memset(g_upload, 0, sizeof(g_upload));
    memset(g_upload_256, 0, sizeof(g_upload_256));
    memset(g_launcher_upload, 0, sizeof(g_launcher_upload));

    ps2_video_packets_reset(1);

    SyncDCache((void *)g_upload,
               (void *)((unsigned char *)g_upload + sizeof(g_upload)));

    ps2_video_packets_hard_reset_redraw_textures(g_upload);

    packet = packet_init(32, PACKET_NORMAL);
    if (!packet)
        return;

    q = packet->data;
    q = draw_setup_environment(q, 0, &g_frame, &g_z);
    q = draw_clear(q, 0, 0.0f, 0.0f, (float)g_frame.width, (float)g_frame.height, 0, 0, 0);
    q = draw_finish(q);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
    dma_wait_fast();
    packet_free(packet);
}
