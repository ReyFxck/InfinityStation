#include "launcher_video.h"
#include "video.h"
#include "video_internal.h"

#include "../ui/launcher/background/background.h"

#include <string.h>
#include <kernel.h>
#include <dma.h>
#include <draw.h>
#include <draw_tests.h>
#include <graph.h>
#include <packet.h>
#include <gs_psm.h>
#include <gs_gp.h>
#include <gif_tags.h>


static int g_launcher_video_ready = 0;
static texbuffer_t g_launcher_tex;
static packet_t *g_launcher_tex_packet = 0;
static packet_t *g_launcher_draw_packet = 0;

static framebuffer_t g_launcher_frame;
static zbuffer_t g_launcher_z;

/* TEXA register tells the GS how to interpret 1-bit alpha textures
 * (which is what we use for the launcher: GS_PSM_16). With AEM=0:
 *   - texel A bit = 0  ->  fragment alpha = TA0
 *   - texel A bit = 1  ->  fragment alpha = TA1
 *
 * We pick TA0=0x00 (treat "untouched" texels as fully transparent) and
 * TA1=0x80 (treat "drawn" texels as opaque enough to pass alpha test).
 * The alpha test is "alpha != 0 -> draw, else keep framebuffer", so
 * drawn UI pixels render and untouched ones reveal whatever was drawn
 * BEFORE the textured sprite in the same packet (i.e. the starfield). */
static qword_t *launcher_emit_texa(qword_t *q, uint8_t ta0,
                                   uint8_t aem, uint8_t ta1)
{
    q->dw[0] = GIF_SET_TAG(1, 1, 0, 0, GIF_FLG_PACKED, 1);
    q->dw[1] = GIF_REG_AD;
    q++;
    q->dw[0] = GS_SET_TEXA(ta0, aem, ta1);
    q->dw[1] = GS_REG_TEXA;
    q++;
    return q;
}

int ps2_launcher_video_init_once(void)
{
    packet_t *packet;
    qword_t *q;
    lod_t lod;
    clutbuffer_t clut;
    /* The draw packet has to fit, in the worst case:
     *   draw_setup_environment        ~6  qw
     *   launcher_emit_texa             2  qw
     *   draw_clear                     6  qw
     *   draw_pixel_test (enable)       2  qw
     *   STAR_COUNT * draw_rect_filled  ~5 qw each = ~1280 qw for 256
     *   draw_texture_sampling          2  qw
     *   draw_texturebuffer             2  qw
     *   draw_rect_textured             5  qw
     *   draw_pixel_test (restore)      2  qw
     *   draw_finish                    2  qw
     * which is comfortably under 4096 qw. The previous 1024-qw packet
     * would overflow as soon as we started emitting per-star sprite
     * primitives. */
    const int draw_packet_qwords = 4096;
    const int tex_packet_qwords  =
        (((PS2_LAUNCHER_WIDTH * PS2_LAUNCHER_HEIGHT * 2u) + 15u) / 16u) + 512u;

    if (g_launcher_video_ready)
        return 1;

    memset(&g_launcher_frame, 0, sizeof(g_launcher_frame));
    g_launcher_frame.width = ps2_video_frame_width();
    g_launcher_frame.height = ps2_video_frame_height();
    g_launcher_frame.mask = 0;
    g_launcher_frame.psm = GS_PSM_32;
    g_launcher_frame.address = ps2_video_frame_address();

    memset(&g_launcher_z, 0, sizeof(g_launcher_z));
    g_launcher_z.enable = DRAW_DISABLE;

    memset(&g_launcher_tex, 0, sizeof(g_launcher_tex));
    g_launcher_tex.width = 1024;
    g_launcher_tex.psm = GS_PSM_16;
    g_launcher_tex.address = graph_vram_allocate(1024, 512, GS_PSM_16, GRAPH_ALIGN_BLOCK);
    g_launcher_tex.info.width = draw_log2(1024);
    g_launcher_tex.info.height = draw_log2(512);
    /* RGBA (TCC=1): the GS samples the texel's 1-bit alpha and feeds
     * it through TEXA into the fragment alpha. Without this the
     * alpha channel of the texture is ignored and our alpha test
     * (used to make the upload buffer transparent over the
     * starfield) would never trigger. */
    g_launcher_tex.info.components = TEXTURE_COMPONENTS_RGBA;
    g_launcher_tex.info.function = TEXTURE_FUNCTION_DECAL;

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

    packet = packet_init(32, PACKET_NORMAL);
    if (!packet)
        return 0;

    q = packet->data;
    q = draw_texture_sampling(q, 0, &lod);
    q = draw_texturebuffer(q, 0, &g_launcher_tex, &clut);
    q = draw_finish(q);

    dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
    dma_wait_fast();
    packet_free(packet);

    g_launcher_tex_packet = packet_init(tex_packet_qwords, PACKET_NORMAL);
    g_launcher_draw_packet = packet_init(draw_packet_qwords, PACKET_NORMAL);

    if (!g_launcher_tex_packet || !g_launcher_draw_packet)
        return 0;

    g_launcher_video_ready = 1;
    return 1;
}

void ps2_launcher_video_begin_frame(uint16_t clear_color)
{
    /* The launcher upload buffer is now used as a 1-bit-alpha overlay
     * on top of the GS-drawn starfield: pixels we don't touch must
     * have A=0 so the textured-rect alpha test (configured in
     * end_frame) skips them and the stars show through. We MUST clear
     * to literal 0x0000 (alpha bit clear), NOT to the LUT-converted
     * value of black -- LUT conversion sets the alpha bit, which
     * would make the whole texture opaque and hide the starfield
     * entirely.
     *
     * The `clear_color` argument is kept for API stability but is
     * only honored when non-zero (callers wanting an opaque colored
     * background can still ask for it explicitly). */
    if (clear_color == 0) {
        memset(g_launcher_upload, 0, sizeof(g_launcher_upload));
        return;
    }

    {
        unsigned y;
        unsigned x;
        uint16_t conv = ps2_video_convert_rgb565(clear_color);

        for (y = 0; y < PS2_LAUNCHER_HEIGHT; y++) {
            for (x = 0; x < PS2_LAUNCHER_WIDTH; x++) {
                g_launcher_upload[y][x] = conv;
            }
        }
    }
}

void ps2_launcher_video_put_pixel(unsigned x, unsigned y, uint16_t color)
{
    if (x >= PS2_LAUNCHER_WIDTH || y >= PS2_LAUNCHER_HEIGHT)
        return;

    g_launcher_upload[y][x] = ps2_video_convert_rgb565(color);
}

void ps2_launcher_video_end_frame(void)
{
    qword_t *q;
    texrect_t rect;
    lod_t lod;
    clutbuffer_t clut;
    atest_t atest;
    dtest_t dtest;
    ztest_t ztest;

    dma_wait_fast();
    SyncDCache(g_launcher_upload,
               (void *)((unsigned char *)g_launcher_upload + sizeof(g_launcher_upload)));

    q = g_launcher_tex_packet->data;
    q = draw_texture_transfer(q,
                              g_launcher_upload,
                              PS2_LAUNCHER_WIDTH,
                              PS2_LAUNCHER_HEIGHT,
                              GS_PSM_16,
                              g_launcher_tex.address,
                              g_launcher_tex.width);
    q = draw_texture_flush(q);

    dma_channel_send_chain(DMA_CHANNEL_GIF,
                           g_launcher_tex_packet->data,
                           q - g_launcher_tex_packet->data,
                           0,
                           0);
    dma_wait_fast();

    memset(&rect, 0, sizeof(rect));
    rect.v0.x = 4.0f;
    rect.v0.y = 4.0f;
    rect.v0.z = 0;

    rect.v1.x = 636.0f;
    rect.v1.y = 444.0f;
    rect.v1.z = 0;

    rect.t0.u = 0.5f;
    rect.t0.v = 0.5f;
    rect.t1.u = (float)PS2_LAUNCHER_WIDTH - 1.5f;
    rect.t1.v = (float)PS2_LAUNCHER_HEIGHT - 1.5f;

    rect.color.r = 0x80;
    rect.color.g = 0x80;
    rect.color.b = 0x80;
    rect.color.a = 0x80;
    rect.color.q = 1.0f;

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

    /* Alpha test config used when drawing the UI texture sprite:
     *  - texel A=0 (regions that put_pixel never touched, i.e. the
     *    transparent background of the upload buffer) -> mapped to
     *    fragment alpha 0x00 by TEXA -> NOTEQUAL 0 fails -> KEEP, so
     *    the framebuffer (which already contains the starfield) is
     *    preserved.
     *  - texel A=1 (UI elements drawn via put_pixel) -> fragment
     *    alpha 0x80 -> NOTEQUAL 0 passes -> draw the texture color.
     */
    memset(&atest, 0, sizeof(atest));
    atest.enable = DRAW_ENABLE;
    atest.method = ATEST_METHOD_NOTEQUAL;
    atest.compval = 0x00;
    /* KEEP_ALL: when alpha test fails, neither frame nor z buffer is
     * updated. Z is disabled anyway in the launcher, but this matches
     * the intended "skip these texels entirely" semantics. */
    atest.keep = ATEST_KEEP_ALL;

    memset(&dtest, 0, sizeof(dtest));
    dtest.enable = DRAW_DISABLE;
    dtest.pass = DTEST_METHOD_PASS_ZERO;

    memset(&ztest, 0, sizeof(ztest));
    ztest.enable = DRAW_ENABLE;
    ztest.method = ZTEST_METHOD_ALLPASS;

    q = g_launcher_draw_packet->data;
    q = draw_setup_environment(q, 0, &g_launcher_frame, &g_launcher_z);
    q = launcher_emit_texa(q, 0x00, 0, 0x80);
    q = draw_clear(q, 0, 0.0f, 0.0f, (float)g_launcher_frame.width,
                   (float)g_launcher_frame.height, 0, 0, 0);

    /* Stars are drawn on top of the cleared framebuffer, BEFORE the
     * textured UI sprite. Alpha test stays disabled here so all star
     * sprites render unconditionally. */
    q = launcher_background_emit_packet(q);

    /* Now switch on the alpha test and draw the UI texture: the
     * transparent regions reveal the stars beneath. */
    q = draw_pixel_test(q, 0, &atest, &dtest, &ztest);
    q = draw_texture_sampling(q, 0, &lod);
    q = draw_texturebuffer(q, 0, &g_launcher_tex, &clut);
    q = draw_rect_textured(q, 0, &rect);
    q = draw_finish(q);

    graph_wait_vsync();

    dma_channel_send_normal(DMA_CHANNEL_GIF,
                            g_launcher_draw_packet->data,
                            q - g_launcher_draw_packet->data,
                            0,
                            0);
    draw_wait_finish();
}
