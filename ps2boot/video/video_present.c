#include "video_internal.h"
#include "app/overlay.h"

/*
 * snes9x agora compila com PIXEL_FORMAT=BGR555 (definido pelo flag
 * INF_PIXEL_BGR555 do Makefile). BGR555 e' (B<<10)|(G<<5)|R em 15
 * bits -- exatamente o layout do GS_PSMCT16, com o bit 15 livre.
 *
 * Como o draw packet do video usa TEXTURE_COMPONENTS_RGB (TCC=RGB),
 * o GS sempre ignora o bit de alpha do texel; entao' as funcoes
 * deste arquivo nao precisam mais de conversao por pixel: o frame
 * que o snes9x escreveu ja' esta' no layout que a GS quer.
 *
 * Antes desta mudanca, todo frame de gameplay aplicava uma LUT de
 * 128KB (R/B swap + alpha=1) em ~57k pixels. A LUT excedia o D-cache
 * de 16KB do EE em 8x, e cada lookup virava miss em RAM. Trocando
 * por compute inline (PR #35) ja' melhorou; eliminar a conversao
 * por completo (este PR) reduz o caminho a memcpy.
 */

static inline void ps2_video_copy_pitched_stride(
    const uint8_t *src_bytes, unsigned width, unsigned height, size_t pitch,
    uint16_t *dst_base, unsigned dst_stride
)
{
    size_t row_bytes = (size_t)width * sizeof(uint16_t);
    unsigned y;

    for (y = 0; y < height; y++) {
        const uint16_t *src = (const uint16_t *)(src_bytes + (y * pitch));
        uint16_t *dst = dst_base + (y * dst_stride);
        memcpy(dst, src, row_bytes);
    }
}

static inline void ps2_video_copy_linear(
    const uint16_t *src, uint16_t *dst, unsigned count
)
{
    memcpy(dst, src, (size_t)count * sizeof(uint16_t));
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

        if (width == PS2_VIDEO_UPLOAD_256_WIDTH &&
            pitch == (PS2_VIDEO_UPLOAD_256_WIDTH * sizeof(uint16_t))) {
            ps2_video_copy_linear(
                (const uint16_t *)src,
                g_upload_256,
                width * height
            );
        } else {
            ps2_video_copy_pitched_stride(
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
            t_ovl = ps2_video_prof_delta(ps2_video_prof_read_count(), t_ovl);
        }

        t1 = ps2_video_prof_read_count();

        ps2_video_packets_upload_and_draw(
            upload_src_256,
            PS2_VIDEO_UPLOAD_256_WIDTH,
            height,
            width,
            height,
            wait_vblanks
        );

        t2 = ps2_video_prof_read_count();

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
        ps2_video_copy_linear(
            (const uint16_t *)src,
            g_upload,
            width * height
        );
    } else {
        ps2_video_copy_pitched_stride(
            src,
            width,
            height,
            pitch,
            g_upload,
            PS2_VIDEO_TEX_WIDTH
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

    ps2_video_copy_pitched_stride(
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
