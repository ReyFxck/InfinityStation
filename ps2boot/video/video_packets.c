#include "video_internal.h"
#include <kernel.h>

#define PS2_VIDEO_DRAW_BASE_PACKET_QWORDS 64u
#define PS2_VIDEO_RECT_TEMPLATE_COUNT 4u

static lod_t g_lod_nearest;
static clutbuffer_t g_clut_none;

static qword_t g_draw_base_packets[PS2_VIDEO_TEX_SLOTS][PS2_VIDEO_DRAW_BASE_PACKET_QWORDS];
static texrect_t g_rect_templates[PS2_VIDEO_RECT_TEMPLATE_COUNT];
static unsigned g_draw_base_qwcs[PS2_VIDEO_TEX_SLOTS];
static texbuffer_t g_tex_slots[PS2_VIDEO_TEX_SLOTS];
static unsigned g_tex_slot_next;
static unsigned g_tex_slots_in_flight;
static unsigned g_last_uploaded_tex_slot = 0;
static int g_last_uploaded_tex_valid = 0;
static unsigned g_last_band_clear_mode = 0xffffffffu;

static const float g_aspect_rects[][4] = {
    { 64.0f,  0.0f, 576.0f, 448.0f }, /* PS2_ASPECT_4_3  */
    {  0.0f, 44.0f, 640.0f, 404.0f }, /* PS2_ASPECT_16_9 */
    {  0.0f,  0.0f, 640.0f, 448.0f }, /* PS2_ASPECT_FULL */
    { 96.0f, 28.0f, 544.0f, 420.0f }  /* PS2_ASPECT_PIXEL */
};

static inline unsigned ps2_video_get_effective_aspect_mode(void)
{
    unsigned mode = (unsigned)g_aspect_mode;

    if (mode > PS2_ASPECT_PIXEL)
        mode = PS2_ASPECT_4_3;

    return mode;
}

static void ps2_video_build_rect_template(unsigned mode)
{
    texrect_t *rect;

    if (mode >= PS2_VIDEO_RECT_TEMPLATE_COUNT)
        return;

    rect = &g_rect_templates[mode];

    rect->v0.x = g_aspect_rects[mode][0];
    rect->v0.y = g_aspect_rects[mode][1];
    rect->v0.z = 0;
    rect->v1.x = g_aspect_rects[mode][2];
    rect->v1.y = g_aspect_rects[mode][3];
    rect->v1.z = 0;

    rect->t0.u = 0.0f;
    rect->t0.v = 0.0f;
    rect->t1.u = 0.0f;
    rect->t1.v = 0.0f;

    rect->color.r = 0x80;
    rect->color.g = 0x80;
    rect->color.b = 0x80;
    rect->color.a = 0x80;
    rect->color.q = 1.0f;
}

static void ps2_video_build_all_rect_templates(void)
{
    unsigned i;

    for (i = 0; i < PS2_VIDEO_RECT_TEMPLATE_COUNT; i++)
        ps2_video_build_rect_template(i);
}

static inline const texrect_t *ps2_video_get_rect_template(void)
{
    return &g_rect_templates[ps2_video_get_effective_aspect_mode()];
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

int ps2_video_packets_init(void)
{
    packet_t *packet;
    qword_t *q;
    unsigned i;

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

    for (i = 0; i < PS2_VIDEO_TEX_SLOTS; i++) {
        g_tex_packets[i] = packet_init(
            (((((PS2_VIDEO_TEX_WIDTH * PS2_VIDEO_TEX_HEIGHT * 2u) + 15u) / 16u) + 256u)),
            PACKET_NORMAL
        );
        g_draw_packets[i] = packet_init(1024, PACKET_NORMAL);
        if (!g_tex_packets[i] || !g_draw_packets[i])
            return 0;
    }

    ps2_video_build_all_draw_base_packets();
    ps2_video_build_all_rect_templates();
    g_last_band_clear_mode = 0xffffffffu;

    return 1;
}

void ps2_video_packets_reset(int hard)
{
    g_tex = g_tex_slots[0];
    g_tex_slot_next = 0;
    g_tex_slots_in_flight = 0;
    g_last_uploaded_tex_slot = 0;
    g_last_uploaded_tex_valid = 0;
    g_last_band_clear_mode = 0xffffffffu;

    if (hard) {
        ps2_video_build_all_draw_base_packets();
        ps2_video_build_all_rect_templates();
    }
}

int ps2_video_packets_last_upload_valid(void)
{
    return g_last_uploaded_tex_valid;
}

void ps2_video_packets_hard_reset_redraw_textures(const uint16_t *src)
{
    packet_t *tex_packet;
    qword_t *q;
    unsigned i;

    for (i = 0; i < PS2_VIDEO_TEX_SLOTS; i++) {
        tex_packet = g_tex_packets[i];
        if (!tex_packet)
            return;

        q = tex_packet->data;
        q = draw_texture_transfer(
            q,
            (void *)src,
            PS2_VIDEO_TEX_WIDTH,
            PS2_VIDEO_TEX_HEIGHT,
            GS_PSM_16,
            g_tex_slots[i].address,
            g_tex_slots[i].width
        );
        q = draw_texture_flush(q);
        dma_channel_send_chain(DMA_CHANNEL_GIF, tex_packet->data, q - tex_packet->data, 0, 0);
        dma_wait_fast();
    }
}

void ps2_video_packets_upload_and_draw(
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
    const texrect_t *base_rect;
    texrect_t rect;
    packet_t *tex_packet;
    unsigned upload_bytes = upload_width * upload_height * sizeof(uint16_t);

    /*
     * O parametro wait_vblanks foi mantido por compatibilidade da API
     * mas NUNCA bloqueia aqui. Se o caller quer sincronizar com vsync,
     * ele faz isso fora dessa funcao -- via ps2_video_finish_frame()
     * (caminho de gameplay, fora de retro_run) ou chamando
     * graph_wait_vsync() diretamente (caminho de menu).
     *
     * Razao: graph_wait_vsync DENTRO do upload_and_draw ficava DENTRO
     * de retro_run (porque upload_and_draw e' chamado pelo video_cb),
     * o que tornava impossivel medir tempo real de trabalho do EE sem
     * incluir a espera de vblank (~16.6 ms NTSC).
     */
    (void)wait_vblanks;

    if (g_tex_slots_in_flight >= PS2_VIDEO_TEX_SLOTS) {
        dma_wait_fast();
        draw_wait_finish();
        g_tex_slots_in_flight = 0;
    }

    tex_slot = g_tex_slot_next;
    g_tex = g_tex_slots[tex_slot];
    g_tex_slot_next = (g_tex_slot_next + 1u) % PS2_VIDEO_TEX_SLOTS;
    tex_packet = g_tex_packets[tex_slot];

    if (!tex_packet)
        return;

    base_rect = ps2_video_get_rect_template();

    SyncDCache((void *)upload, (void *)((const unsigned char *)upload + upload_bytes));

    /*
     * Tudo num chain DMA so': transferencia da textura + texflush +
     * setup do GS + draw rect + finish. Antes era 2 DMAs separados
     * (chain pra textura, normal pro draw), com dma_wait_fast() no
     * meio pra serializar o canal GIF -- esse wait estourava o EE em
     * ~0.5-1 ms/frame esperando a textura ser entregue ao GS antes de
     * disparar o draw.
     *
     * Agora todos os GIFtags vao no mesmo source-chain com tags
     * DMATAG_CNT/DMATAG_END, e o DMAC processa em sequencia sem
     * intervencao do EE. Continua tendo serializacao no canal GIF
     * (PATH3), so' que ela acontece no DMAC, nao bloqueando a EE.
     *
     * Tex_packet ja' e' grande o suficiente: alocacao de 512x256
     * PSMCT16 + 256 qwords slack ~= 16640 qwords. O chain real usa
     * so' os DMA tags de header (textura vai por DMATAG_REF lendo
     * direto do buffer source, nao copiada pra dentro do packet),
     * mais o chunk de draw, que rara passa de ~80 qwords.
     */
    q = tex_packet->data;
    q = draw_texture_transfer(
        q,
        (void *)upload,
        upload_width,
        upload_height,
        GS_PSM_16,
        g_tex.address,
        g_tex.width
    );

    /*
     * Substitui draw_texture_flush por uma versao "CNT" (continua a
     * cadeia em vez de DMATAG_END), pra a cadeia seguir pro draw rect
     * abaixo. O conteudo dos GIFtags e' o mesmo: TEXFLUSH=1.
     */
    DMATAG_CNT(q, 2, 0, 0, 0);
    q++;
    PACK_GIFTAG(q, GIF_SET_TAG(1, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;
    PACK_GIFTAG(q, 1, GS_REG_TEXFLUSH);
    q++;

    rect = *base_rect;
    rect.t1.u = (float)width - 1.0f;
    rect.t1.v = (float)height - 1.0f;

    /*
     * Tag DMATAG_END que cobre o restante do packet (setup + draw +
     * finish). qwc final preenchido depois que sabemos quantos qwords
     * o draw produziu.
     */
    {
        qword_t *draw_dmatag = q;
        qword_t *draw_start;
        unsigned draw_qwc;

        q++;
        draw_start = q;

        if (g_draw_base_qwcs[tex_slot] != 0) {
            memcpy(
                q,
                g_draw_base_packets[tex_slot],
                g_draw_base_qwcs[tex_slot] * sizeof(qword_t)
            );
            q += g_draw_base_qwcs[tex_slot];
        } else {
            q = draw_setup_environment(q, 0, &g_frame, &g_z);
            q = draw_texture_sampling(q, 0, &g_lod_nearest);
            q = draw_texturebuffer(q, 0, &g_tex, &g_clut_none);
        }

        {
            unsigned aspect_mode = ps2_video_get_effective_aspect_mode();

            if (aspect_mode != PS2_ASPECT_FULL &&
                g_last_band_clear_mode != aspect_mode) {
                q = ps2_video_clear_bands(
                    q,
                    base_rect->v0.x,
                    base_rect->v0.y,
                    base_rect->v1.x,
                    base_rect->v1.y
                );
            }

            g_last_band_clear_mode = aspect_mode;
        }

        q = draw_rect_textured(q, 0, &rect);
        q = draw_finish(q);

        draw_qwc = (unsigned)(q - draw_start);
        DMATAG_END(draw_dmatag, draw_qwc, 0, 0, 0);
    }

    /*
     * Sem vsync wait aqui -- responsabilidade do caller (ver
     * ps2_video_finish_frame pro caminho de gameplay).
     */
    dma_wait_fast();
    dma_channel_send_chain(DMA_CHANNEL_GIF, tex_packet->data, q - tex_packet->data, 0, 0);

    g_last_uploaded_tex_slot = tex_slot;
    g_last_uploaded_tex_valid = 1;

    if (g_tex_slots_in_flight < PS2_VIDEO_TEX_SLOTS)
        g_tex_slots_in_flight++;
}

void ps2_video_packets_redraw_last(unsigned width, unsigned height, unsigned wait_vblanks)
{
    qword_t *q;
    const texrect_t *base_rect;
    texrect_t rect;
    packet_t *draw_packet;
    unsigned tex_slot;

    if (!g_video_ready || !g_last_uploaded_tex_valid)
        return;

    tex_slot = g_last_uploaded_tex_slot;
    if (tex_slot >= PS2_VIDEO_TEX_SLOTS)
        return;

    draw_packet = g_draw_packets[tex_slot];
    if (!draw_packet)
        return;

    g_tex = g_tex_slots[tex_slot];
    base_rect = ps2_video_get_rect_template();
    rect = *base_rect;
    rect.t1.u = (float)width - 1.0f;
    rect.t1.v = (float)height - 1.0f;

    dma_wait_fast();

    if (g_draw_base_qwcs[tex_slot] != 0) {
        memcpy(
            draw_packet->data,
            g_draw_base_packets[tex_slot],
            g_draw_base_qwcs[tex_slot] * sizeof(qword_t)
        );
        q = draw_packet->data + g_draw_base_qwcs[tex_slot];
    } else {
        q = draw_packet->data;
        q = draw_setup_environment(q, 0, &g_frame, &g_z);
        q = draw_texture_sampling(q, 0, &g_lod_nearest);
        q = draw_texturebuffer(q, 0, &g_tex, &g_clut_none);
    }

    {
        unsigned aspect_mode = ps2_video_get_effective_aspect_mode();

        if (aspect_mode != PS2_ASPECT_FULL &&
            g_last_band_clear_mode != aspect_mode) {
            q = ps2_video_clear_bands(
                q,
                base_rect->v0.x,
                base_rect->v0.y,
                base_rect->v1.x,
                base_rect->v1.y
            );
        }

        g_last_band_clear_mode = aspect_mode;
    }

    q = draw_rect_textured(q, 0, &rect);
    q = draw_finish(q);

    /* wait_vblanks ignorado: vsync agora e' responsabilidade do caller. */
    (void)wait_vblanks;

    dma_wait_fast();

    dma_channel_send_normal(DMA_CHANNEL_GIF, draw_packet->data, q - draw_packet->data, 0, 0);
}
