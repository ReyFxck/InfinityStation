#include "video_internal.h"
#include "app/overlay.h"

/*
 * Caminho de gameplay (apos PR #36): snes9x compila com PIXEL_FORMAT=BGR555,
 * que e' (B<<10)|(G<<5)|R em 15 bits -- exatamente o layout do GS_PSMCT16,
 * com o bit 15 livre. Como o draw packet usa TEXTURE_COMPONENTS_RGB (TCC=RGB),
 * o GS sempre ignora o bit de alpha do texel; o frame que o snes9x escreveu
 * ja' esta' no layout que a GS quer.
 *
 * Esta presentacao DMA-eia o buffer da snes9x DIRETO pro GS, sem copiar pra
 * um buffer intermediario.
 *
 *   - GFX.Pitch = IMAGE_WIDTH * 2 = 512 px * 2 byte = 1024 bytes/linha
 *   - O upload tem stride exatamente igual a' largura da textura GS (512).
 *     Como o draw_texture_transfer le' a memoria fonte de forma contigua
 *     e o pitch da snes9x bate com a largura da textura GS (512 px),
 *     pode-se passar src direto.
 *   - O snes9x ja' chama SyncDCache no GFX.Screen antes de video_cb, ent~ao
 *     a textura nao precisa de write-back de cache aqui.
 *   - Jogos 256-wide so' renderizam pixels 0-255 em cada linha; o lado
 *     direito (256-511) fica com lixo de frames anteriores, mas nunca eh
 *     amostrado porque draw_rect_textured usa UV [0, 256) com NEAREST e
 *     CLAMP. Jogos hi-res 512-wide preenchem a linha toda.
 *   - Overlay (FPS counter etc.) eh' poke direto em GFX.Screen via cast
 *     -- snes9x reescreve as mesmas posicoes no proximo frame, ent~ao o
 *     overlay reaparece a cada chamada via dbg_overlay().
 *
 * Antes deste caminho havia um memcpy da snes9x pra g_upload_256/g_upload
 * (~112KB/frame). Em ~57k pixels eram ~0.7-1ms/frame de EE. Eliminar
 * libera o EE pra rodar o core do snes9x.
 */

void ps2_video_present_rgb565(const void *data, unsigned width, unsigned height, size_t pitch)
{
    uint16_t *src;
    int wait_vsync;
    unsigned wait_vblanks;
    int overlay_active;
    unsigned upload_stride;
    unsigned t0, t1, t2, t_ovl;

    if (!g_video_ready || !data || width == 0 || height == 0 || pitch == 0)
        return;

    if ((pitch & 0xFu) != 0)
        return; /* fonte tem que ser alinhada a qword pra DMA-direct */

    upload_stride = (unsigned)(pitch / sizeof(uint16_t));

    if (upload_stride == 0 || upload_stride > PS2_VIDEO_TEX_WIDTH)
        return;

    if (width > upload_stride)
        width = upload_stride;

    if (height > PS2_VIDEO_TEX_HEIGHT)
        height = PS2_VIDEO_TEX_HEIGHT;

    /*
     * src vem const do libretro (assinatura do retro_video_refresh_t),
     * mas o snes9x e' o owner real do GFX.Screen e nao re-le' depois
     * do video_cb -- so' reescreve no proximo frame. O overlay precisa
     * pokar pixels nele, ent~ao vamos remover o const.
     */
    src = (uint16_t *)data;

    wait_vsync = select_menu_actions_game_vsync_enabled();
    wait_vblanks = app_overlay_video_wait_vblanks(wait_vsync);
    overlay_active = (g_dbg1[0] || g_dbg2[0] || g_dbg3[0] || g_dbg4[0]);

    t0 = ps2_video_prof_read_count();

    t_ovl = 0;
    if (overlay_active) {
        t_ovl = ps2_video_prof_read_count();
        dbg_set_target(src, upload_stride, width, height);
        dbg_overlay();
        dbg_reset_target();
        t_ovl = ps2_video_prof_delta(ps2_video_prof_read_count(), t_ovl);
        /*
         * Os pixels escritos pelo overlay ficam dirty na cache do EE.
         * O ps2_video_packets_upload_and_draw chama SyncDCache no upload
         * antes de disparar o DMA, ent~ao essa flush la' tambem cobre
         * as escritas do overlay. Nao precisa flush explicito aqui.
         */
    }

    t1 = ps2_video_prof_read_count();

    /*
     * Submete o present (DMA do tex + DMA do draw packet) sem bloquear
     * em vsync. O wait_vblanks fica acumulado em g_video_pending_vblanks
     * pra ser consumido em ps2_video_finish_frame() do main loop -- fora
     * de retro_run, pra liberar a janela de medicao de tempo de trabalho.
     */
    ps2_video_packets_upload_and_draw(
        src,
        upload_stride,
        height,
        width,
        height,
        0
    );

    g_video_pending_vblanks += wait_vblanks;

    t2 = ps2_video_prof_read_count();

    ps2_video_prof_commit(
        (unsigned)(ps2_video_prof_delta(t1, t0) - t_ovl),
        t_ovl,
        ps2_video_prof_delta(t2, t1),
        ps2_video_prof_delta(t2, t0), width, height);
}

void ps2_video_present_ui_fixed_rgb565(const void *data, unsigned width, unsigned height, size_t pitch)
{
    const uint8_t *src_bytes;
    unsigned y;
    size_t row_bytes;

    if (!g_video_ready || !data || width == 0 || height == 0)
        return;

    if (width > PS2_VIDEO_UPLOAD_256_WIDTH)
        width = PS2_VIDEO_UPLOAD_256_WIDTH;

    if (height > PS2_VIDEO_TEX_HEIGHT)
        height = PS2_VIDEO_TEX_HEIGHT;

    src_bytes = (const uint8_t *)data;
    row_bytes = (size_t)width * sizeof(uint16_t);

    for (y = 0; y < height; y++) {
        const uint16_t *row = (const uint16_t *)(src_bytes + (y * pitch));
        memcpy(&g_upload_256[y * PS2_VIDEO_UPLOAD_256_WIDTH], row, row_bytes);
    }

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
        0
    );

    /*
     * Caminho do menu: o vsync ainda e' inline porque essa funcao
     * NAO e' chamada de dentro de retro_run -- entao manter o wait
     * aqui simplifica o caller (ps2_menu_draw nao precisa lembrar
     * de sincronizar). O custo extra e' nulo: ps2_video_finish_frame
     * so' drena g_video_pending_vblanks, que aqui nao incrementamos.
     */
    while (wait_vblanks != 0) {
        graph_wait_vsync();
        wait_vblanks--;
    }
}
