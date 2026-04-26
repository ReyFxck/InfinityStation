#include "video_internal.h"
#include "app/overlay.h"

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

    if (width <= PS2_VIDEO_UPLOAD_256_WIDTH) {
        const uint16_t *upload_src_256 = g_upload_256;
        int reusable_linear_256 = 0;

        if (width == PS2_VIDEO_UPLOAD_256_WIDTH &&
            pitch == (PS2_VIDEO_UPLOAD_256_WIDTH * sizeof(uint16_t)) &&
            !overlay_active) {
            upload_src_256 = (const uint16_t *)src;
            reusable_linear_256 = 1;
        } else if (width == PS2_VIDEO_UPLOAD_256_WIDTH &&
                   pitch == (PS2_VIDEO_UPLOAD_256_WIDTH * sizeof(uint16_t))) {
            ps2_video_convert_rgb565_linear(
                (const uint16_t *)src,
                g_upload_256,
                width * height
            );
        } else {
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
            if (upload_src_256 != g_upload_256) {
                memcpy(g_upload_256, upload_src_256, width * height * sizeof(uint16_t));
                upload_src_256 = g_upload_256;
            }

            t_ovl = ps2_video_prof_read_count();
            dbg_set_target(
                g_upload_256,
                PS2_VIDEO_UPLOAD_256_WIDTH,
                width,
                height
            );
            dbg_overlay();
            dbg_reset_target();
            t_ovl = ps2_video_prof_delta(ps2_video_prof_read_count(), t_ovl);
        }

        t1 = ps2_video_prof_read_count();

        if (reusable_linear_256 &&
            ps2_video_cache_can_reuse_256(upload_src_256, width, height)) {
            ps2_video_packets_redraw_last(width, height, wait_vblanks);
            t2 = ps2_video_prof_read_count();
            ps2_video_prof_commit(
                (unsigned)(ps2_video_prof_delta(t1, t0) - t_ovl),
                t_ovl,
                ps2_video_prof_delta(t2, t1),
                ps2_video_prof_delta(t2, t0), width, height);
            return;
        }

        ps2_video_packets_upload_and_draw(
            upload_src_256,
            PS2_VIDEO_UPLOAD_256_WIDTH,
            height,
            width,
            height,
            wait_vblanks
        );

        t2 = ps2_video_prof_read_count();

        if (reusable_linear_256)
            ps2_video_cache_store_256(upload_src_256, width, height);
        else
            ps2_video_cache_invalidate();

        ps2_video_prof_commit(
                (unsigned)(ps2_video_prof_delta(t1, t0) - t_ovl),
                t_ovl,
                ps2_video_prof_delta(t2, t1),
                ps2_video_prof_delta(t2, t0), width, height);
        return;
    }

    if (width == PS2_VIDEO_TEX_WIDTH &&
        pitch == (PS2_VIDEO_TEX_WIDTH * sizeof(uint16_t))) {
        ps2_video_convert_rgb565_linear(
            (const uint16_t *)src,
            g_upload,
            width * height
        );
    } else {
        ps2_video_convert_rgb565_pitched(
            src,
            width,
            height,
            pitch,
            g_upload
        );
    }

    ps2_video_cache_invalidate();

    t_ovl = 0;
    if (overlay_active) {
        t_ovl = ps2_video_prof_read_count();
        dbg_set_target(g_upload, PS2_VIDEO_TEX_WIDTH, width, height);
        dbg_overlay();
        dbg_reset_target();
        t_ovl = ps2_video_prof_delta(ps2_video_prof_read_count(), t_ovl);
    }

    t1 = ps2_video_prof_read_count();

    ps2_video_packets_upload_and_draw(
        g_upload,
        PS2_VIDEO_TEX_WIDTH,
        height,
        width,
        height,
        wait_vblanks
    );

    t2 = ps2_video_prof_read_count();

    ps2_video_prof_commit(
                (unsigned)(ps2_video_prof_delta(t1, t0) - t_ovl),
                t_ovl,
                ps2_video_prof_delta(t2, t1),
                ps2_video_prof_delta(t2, t0), width, height);
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

    ps2_video_packets_upload_and_draw(
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

    ps2_video_packets_upload_and_draw(
        g_upload,
        PS2_VIDEO_TEX_WIDTH,
        height,
        width,
        height,
        wait_vblanks
    );
}
