#include "ps2_video_internal.h"
#include "../../../app/app_overlay.h"
#include <stdio.h>
#include <kernel.h>

static lod_t g_lod_nearest;
static clutbuffer_t g_clut_none;
#define PS2_VIDEO_TEX_SLOTS 3u
#define PS2_VIDEO_DRAW_BASE_PACKET_QWORDS 64u

static qword_t g_draw_base_packets[PS2_VIDEO_TEX_SLOTS][PS2_VIDEO_DRAW_BASE_PACKET_QWORDS];
static unsigned g_draw_base_qwcs[PS2_VIDEO_TEX_SLOTS];
static texbuffer_t g_tex_slots[PS2_VIDEO_TEX_SLOTS];
static unsigned g_tex_slot_next;
static unsigned g_tex_slots_in_flight;

static unsigned g_prof_frames;
static unsigned long long g_prof_convert_cycles;
static unsigned long long g_prof_overlay_cycles;
static unsigned long long g_prof_backend_cycles;
static unsigned long long g_prof_total_cycles;

static inline unsigned ps2_video_prof_read_count(void)
{
    unsigned value;
    __asm__ __volatile__("mfc0 %0, $9" : "=r"(value));
    return value;
}

static inline float ps2_video_prof_cycles_to_ms(unsigned long long cycles, unsigned frames)
{
    if (!frames)
        return 0.0f;

    return (float)((double)cycles / (double)frames / 294912.0);
}

static void ps2_video_prof_commit_split(
    unsigned cvt_cycles,
    unsigned ovl_cycles,
    unsigned backend_cycles,
    unsigned total_cycles,
    unsigned width,
    unsigned height
)
{
    g_prof_convert_cycles += cvt_cycles;
    g_prof_overlay_cycles += ovl_cycles;
    g_prof_backend_cycles += backend_cycles;
    g_prof_total_cycles += total_cycles;
    g_prof_frames++;

    if (g_prof_frames >= 30u) {
        snprintf(g_dbg3, 96, "VID cvt %.2f | ovl %.2f | be %.2f",
                 ps2_video_prof_cycles_to_ms(g_prof_convert_cycles, g_prof_frames),
                 ps2_video_prof_cycles_to_ms(g_prof_overlay_cycles, g_prof_frames),
                 ps2_video_prof_cycles_to_ms(g_prof_backend_cycles, g_prof_frames));

        snprintf(g_dbg4, 96, "VID total %.2fms | %ux%u",
                 ps2_video_prof_cycles_to_ms(g_prof_total_cycles, g_prof_frames),
                 width, height);

        g_prof_frames = 0;
        g_prof_convert_cycles = 0;
        g_prof_overlay_cycles = 0;
        g_prof_backend_cycles = 0;
        g_prof_total_cycles = 0;
    }
}


static const float g_aspect_rects[][4] = {
    { 64.0f,  0.0f, 576.0f, 448.0f }, /* PS2_ASPECT_4_3  */
    {  0.0f, 44.0f, 640.0f, 404.0f }, /* PS2_ASPECT_16_9 */
    {  0.0f,  0.0f, 640.0f, 448.0f }, /* PS2_ASPECT_FULL */
    { 96.0f, 28.0f, 544.0f, 420.0f }  /* PS2_ASPECT_PIXEL */
};

static inline void ps2_video_convert_rgb565_linear(
    const uint16_t *src, uint16_t *dst, unsigned count
)
{
    const uint16_t *lut = g_rgb565_lut;
    unsigned i = 0;

    for (; i + 4u <= count; i += 4u) {
        dst[0] = lut[src[0]];
        dst[1] = lut[src[1]];
        dst[2] = lut[src[2]];
        dst[3] = lut[src[3]];
        src += 4;
        dst += 4;
    }

    for (; i < count; i++)
        *dst++ = lut[*src++];
}

static inline void ps2_video_convert_rgb565_pitched_stride(
    const uint8_t *src_bytes, unsigned width, unsigned height, size_t pitch,
    uint16_t *dst_base, unsigned dst_stride
)
{
    const uint16_t *lut = g_rgb565_lut;
    unsigned y;

    for (y = 0; y < height; y++) {
        const uint16_t *src = (const uint16_t *)(src_bytes + (y * pitch));
        uint16_t *dst = dst_base + (y * dst_stride);
        unsigned x = 0;

        for (; x + 4u <= width; x += 4u) {
            dst[0] = lut[src[0]];
            dst[1] = lut[src[1]];
            dst[2] = lut[src[2]];
            dst[3] = lut[src[3]];
            src += 4;
            dst += 4;
        }

        for (; x < width; x++)
            *dst++ = lut[*src++];
    }
}

static inline void ps2_video_convert_rgb565_pitched(
    const uint8_t *src_bytes, unsigned width, unsigned height, size_t pitch, uint16_t *dst_base
)
{
    ps2_video_convert_rgb565_pitched_stride(
        src_bytes, width, height, pitch, dst_base, PS2_VIDEO_TEX_WIDTH
    );
}

static inline void ps2_video_pack_256_from_staging(unsigned width, unsigned height)
{
    unsigned y;

    for (y = 0; y < height; y++) {
        memcpy(
            &g_upload_256[y * PS2_VIDEO_UPLOAD_256_WIDTH],
            &g_upload[y * PS2_VIDEO_TEX_WIDTH],
            width * sizeof(uint16_t)
        );
    }
}

static inline void ps2_video_get_target_rect(float *x0, float *y0, float *x1, float *y1)
{
    unsigned mode = (unsigned)g_aspect_mode;

    if (mode > PS2_ASPECT_PIXEL)
        mode = PS2_ASPECT_4_3;

    *x0 = g_aspect_rects[mode][0];
    *y0 = g_aspect_rects[mode][1];
    *x1 = g_aspect_rects[mode][2];
    *y1 = g_aspect_rects[mode][3];
}

static inline qword_t *ps2_video_clear_bands(
    qword_t *q, float x0, float y0, float x1, float y1
)
{
    if (x0 > 0.0f)
        q = draw_clear(q, 0, 0.0f, 0.0f, x0, (float)g_frame.height, 0, 0, 0);

    if (x1 < (float)g_frame.width)
        q = draw_clear(
            q, 0, x1, 0.0f, (float)g_frame.width, (float)g_frame.height, 0, 0, 0
        );

    if (y0 > 0.0f)
        q = draw_clear(q, 0, x0, 0.0f, x1, y0, 0, 0, 0);

    if (y1 < (float)g_frame.height)
        q = draw_clear(q, 0, x0, y1, x1, (float)g_frame.height, 0, 0, 0);

    return q;
}



static void ps2_video_build_draw_base_packet_for_slot(unsigned slot)
{
    qword_t *q;
    texbuffer_t tex;

    if (slot >= PS2_VIDEO_TEX_SLOTS)
        return;

    tex = g_tex_slots[slot];
    q = g_draw_base_packets[slot];

    q = draw_setup_environment(q, 0, &g_frame, &g_z);
    q = draw_texture_sampling(q, 0, &g_lod_nearest);
    q = draw_texturebuffer(q, 0, &tex, &g_clut_none);

    g_draw_base_qwcs[slot] = (unsigned)(q - g_draw_base_packets[slot]);
}

static void ps2_video_build_all_draw_base_packets(void)
{
    unsigned i;

    for (i = 0; i < PS2_VIDEO_TEX_SLOTS; i++)
        ps2_video_build_draw_base_packet_for_slot(i);
}


void ps2_video_apply_display_offset(void)
{
    graph_set_screen(g_video_off_x, g_video_off_y, g_frame.width, g_frame.height);
}

int ps2_video_init_once(void)
{
    packet_t *packet;
    qword_t *q;
    unsigned i;

    if (g_video_ready)
        return 1;

    ps2_video_build_lut();

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

    for (i = 0; i < PS2_VIDEO_TEX_SLOTS; i++) {
        g_tex_slots[i].width = PS2_VIDEO_TEX_WIDTH;
        g_tex_slots[i].psm = GS_PSM_16;
        g_tex_slots[i].address = graph_vram_allocate(
            PS2_VIDEO_TEX_WIDTH,
            PS2_VIDEO_TEX_HEIGHT,
            GS_PSM_16,
            GRAPH_ALIGN_BLOCK
        );
        g_tex_slots[i].info.width = draw_log2(PS2_VIDEO_TEX_WIDTH);
        g_tex_slots[i].info.height = draw_log2(PS2_VIDEO_TEX_HEIGHT);
        g_tex_slots[i].info.components = TEXTURE_COMPONENTS_RGB;
        g_tex_slots[i].info.function = TEXTURE_FUNCTION_DECAL;
    }

    g_tex = g_tex_slots[0];
    g_tex_slot_next = 0;
    g_tex_slots_in_flight = 0;

    memset(&g_lod_nearest, 0, sizeof(g_lod_nearest));
    g_lod_nearest.calculation = LOD_USE_K;
    g_lod_nearest.max_level = 0;
    g_lod_nearest.mag_filter = LOD_MAG_NEAREST;
    g_lod_nearest.min_filter = LOD_MIN_NEAREST;
    g_lod_nearest.l = 0;
    g_lod_nearest.k = 0.0f;

    memset(&g_clut_none, 0, sizeof(g_clut_none));
    g_clut_none.storage_mode = CLUT_STORAGE_MODE1;
    g_clut_none.load_method = CLUT_NO_LOAD;

    graph_initialize(
        g_frame.address,
        g_frame.width,
        g_frame.height,
        g_frame.psm,
        0,
        0
    );
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

    packet = packet_init(16, PACKET_NORMAL);
    if (!packet)
        return 0;

    q = packet->data;
    q = draw_texture_sampling(q, 0, &g_lod_nearest);
    q = draw_texturebuffer(q, 0, &g_tex, &g_clut_none);
    q = draw_finish(q);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
    dma_wait_fast();
    packet_free(packet);

    g_tex_packet = packet_init(((((PS2_VIDEO_TEX_WIDTH * PS2_VIDEO_TEX_HEIGHT * 2u) + 15u) / 16u) + 256u), PACKET_NORMAL);
    g_draw_packet = packet_init(1024, PACKET_NORMAL);
    if (!g_tex_packet || !g_draw_packet)
        return 0;

    ps2_video_build_all_draw_base_packets();

    g_video_ready = 1;
    return 1;
}

static void ps2_video_upload_and_draw_source(
    const uint16_t *upload,
    unsigned upload_width,
    unsigned upload_height,
    unsigned width,
    unsigned height,
    unsigned wait_vblanks
)
{
    qword_t *q;
    unsigned tex_slot;
    texrect_t rect;
    float x0, y0, x1, y1;
    unsigned upload_bytes = upload_width * upload_height * sizeof(uint16_t);

    if (g_tex_slots_in_flight >= PS2_VIDEO_TEX_SLOTS) {
        draw_wait_finish();
        g_tex_slots_in_flight = 0;
    }

    tex_slot = g_tex_slot_next;
    g_tex = g_tex_slots[tex_slot];
    g_tex_slot_next = (g_tex_slot_next + 1u) % PS2_VIDEO_TEX_SLOTS;

    ps2_video_get_target_rect(&x0, &y0, &x1, &y1);

    dma_wait_fast();
    SyncDCache((void *)upload, (void *)((const unsigned char *)upload + upload_bytes));

    q = g_tex_packet->data;
    q = draw_texture_transfer(
        q,
        (void *)upload,
        upload_width,
        upload_height,
        GS_PSM_16,
        g_tex.address,
        g_tex.width
    );
    q = draw_texture_flush(q);
    dma_channel_send_chain(DMA_CHANNEL_GIF, g_tex_packet->data, q - g_tex_packet->data, 0, 0);

    rect.v0.x = x0;
    rect.v0.y = y0;
    rect.v0.z = 0;
    rect.v1.x = x1;
    rect.v1.y = y1;
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

    if (g_draw_base_qwcs[tex_slot] != 0) {
        memcpy(
            g_draw_packet->data,
            g_draw_base_packets[tex_slot],
            g_draw_base_qwcs[tex_slot] * sizeof(qword_t)
        );
        q = g_draw_packet->data + g_draw_base_qwcs[tex_slot];
    } else {
        q = g_draw_packet->data;
        q = draw_setup_environment(q, 0, &g_frame, &g_z);
        q = draw_texture_sampling(q, 0, &g_lod_nearest);
        q = draw_texturebuffer(q, 0, &g_tex, &g_clut_none);
    }

    q = ps2_video_clear_bands(q, x0, y0, x1, y1);
    q = draw_rect_textured(q, 0, &rect);
    q = draw_finish(q);

    dma_wait_fast();

    while (wait_vblanks != 0) {
        graph_wait_vsync();
        wait_vblanks--;
    }

    dma_channel_send_normal(DMA_CHANNEL_GIF, g_draw_packet->data, q - g_draw_packet->data, 0, 0);

    if (g_tex_slots_in_flight < PS2_VIDEO_TEX_SLOTS)
        g_tex_slots_in_flight++;
}

void ps2_video_present_rgb565(const void *data, unsigned width, unsigned height, size_t pitch)
{
    const uint8_t *src = (const uint8_t *)data;
    int wait_vsync;
    unsigned wait_vblanks;
    int overlay_active;
    unsigned t0, t1, t2, t_ovl;

    if (!g_video_ready || !data || width == 0 || height == 0)
        return;

    if (width > PS2_VIDEO_TEX_WIDTH)
        width = PS2_VIDEO_TEX_WIDTH;
    if (height > PS2_VIDEO_TEX_HEIGHT)
        height = PS2_VIDEO_TEX_HEIGHT;

    wait_vsync = select_menu_actions_game_vsync_enabled();
    wait_vblanks = app_overlay_video_wait_vblanks(wait_vsync);
    overlay_active = (g_dbg1[0] || g_dbg2[0] || g_dbg3[0] || g_dbg4[0]);
    t0 = ps2_video_prof_read_count();

    if (width <= PS2_VIDEO_UPLOAD_256_WIDTH)
    {
        if (width == PS2_VIDEO_UPLOAD_256_WIDTH &&
            pitch == (PS2_VIDEO_UPLOAD_256_WIDTH * sizeof(uint16_t)))
        {
            ps2_video_convert_rgb565_linear(
                (const uint16_t *)src,
                g_upload_256,
                width * height
            );
        }
        else
        {
            ps2_video_convert_rgb565_pitched_stride(
                src,
                width,
                height,
                pitch,
                g_upload_256,
                PS2_VIDEO_UPLOAD_256_WIDTH
            );
        }

        t_ovl = 0;
        if (overlay_active) {
            t_ovl = ps2_video_prof_read_count();
            dbg_set_target(
                g_upload_256,
                PS2_VIDEO_UPLOAD_256_WIDTH,
                width,
                height
            );
            dbg_overlay();
            dbg_reset_target();
            t_ovl = ps2_video_prof_read_count() - t_ovl;
        }

        t1 = ps2_video_prof_read_count();
        ps2_video_upload_and_draw_source(
            g_upload_256,
            PS2_VIDEO_UPLOAD_256_WIDTH,
            height,
            width,
            height,
            wait_vsync
        );
        t2 = ps2_video_prof_read_count();
        ps2_video_prof_commit_split((t1 - t0) - t_ovl, t_ovl, t2 - t1, t2 - t0, width, height);
        return;
    }

    if (width == PS2_VIDEO_TEX_WIDTH &&
        pitch == (PS2_VIDEO_TEX_WIDTH * sizeof(uint16_t)))
    {
        ps2_video_convert_rgb565_linear(
            (const uint16_t *)src,
            g_upload,
            width * height
        );
    }
    else
    {
        ps2_video_convert_rgb565_pitched(
            src,
            width,
            height,
            pitch,
            g_upload
        );
    }

    t_ovl = 0;
    if (overlay_active) {
        t_ovl = ps2_video_prof_read_count();
        dbg_set_target(g_upload, PS2_VIDEO_TEX_WIDTH, width, height);
        dbg_overlay();
        dbg_reset_target();
        t_ovl = ps2_video_prof_read_count() - t_ovl;
    }

    t1 = ps2_video_prof_read_count();
    ps2_video_upload_and_draw_source(
        g_upload,
        PS2_VIDEO_TEX_WIDTH,
        height,
        width,
        height,
        wait_vblanks
    );
    t2 = ps2_video_prof_read_count();
    ps2_video_prof_commit_split((t1 - t0) - t_ovl, t_ovl, t2 - t1, t2 - t0, width, height);
}


void ps2_video_present_ui_fixed_rgb565(const void *data, unsigned width, unsigned height, size_t pitch)
{
    const uint8_t *src = (const uint8_t *)data;

    if (!g_video_ready || !data || width == 0 || height == 0)
        return;

    if (width > PS2_VIDEO_UPLOAD_256_WIDTH)
        width = PS2_VIDEO_UPLOAD_256_WIDTH;

    if (height > PS2_VIDEO_TEX_HEIGHT)
        height = PS2_VIDEO_TEX_HEIGHT;

    ps2_video_convert_rgb565_pitched_stride(
        src, width, height, pitch, g_upload_256, PS2_VIDEO_UPLOAD_256_WIDTH
    );

    ps2_video_upload_and_draw_source(
        g_upload_256,
        PS2_VIDEO_UPLOAD_256_WIDTH,
        height,
        width,
        height,
        0
    );
}

void ps2_video_upload_and_draw_bound(unsigned width, unsigned height, int wait_vsync)
{
    unsigned wait_vblanks = app_overlay_video_wait_vblanks(wait_vsync);

    ps2_video_upload_and_draw_source(
        g_upload,
        PS2_VIDEO_TEX_WIDTH,
        height,
        width,
        height,
        wait_vblanks
    );
}

void ps2_video_hard_reset(void)
{
    packet_t *packet;
    qword_t *q;
    unsigned i;

    if (!g_video_ready)
        return;

    draw_wait_finish();
    dma_wait_fast();

    memset(g_upload, 0, sizeof(g_upload));
    memset(g_frame_base, 0, sizeof(g_frame_base));
    memset(g_upload_256, 0, sizeof(g_upload_256));
    memset(g_launcher_upload, 0, sizeof(g_launcher_upload));

    g_tex = g_tex_slots[0];
    g_tex_slot_next = 0;
    g_tex_slots_in_flight = 0;
    ps2_video_build_all_draw_base_packets();

    SyncDCache((void *)g_upload,
               (void *)((unsigned char *)g_upload + sizeof(g_upload)));

    for (i = 0; i < PS2_VIDEO_TEX_SLOTS; i++) {
        q = g_tex_packet->data;
        q = draw_texture_transfer(
            q,
            g_upload,
            PS2_VIDEO_TEX_WIDTH,
            PS2_VIDEO_TEX_HEIGHT,
            GS_PSM_16,
            g_tex_slots[i].address,
            g_tex_slots[i].width
        );
        q = draw_texture_flush(q);
        dma_channel_send_chain(DMA_CHANNEL_GIF, g_tex_packet->data, q - g_tex_packet->data, 0, 0);
        dma_wait_fast();
    }

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
