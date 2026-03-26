#include "ps2_audio.h"

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <kernel.h>
#include <sifrpc.h>
#include <sifcmd.h>
#include <loadfile.h>
#include <iopheap.h>
#include <sbv_patches.h>

#include "libsdpcm_irx_blob.h"
#include "sjpcm_local.h"

#define SJPCM_CHUNK_FRAMES 800

static SifRpcClientData_t g_cd;
static unsigned int g_sbuff[64] __attribute__((aligned(64)));
static int g_audio_state = 0;
static uintptr_t g_pcmbufl = 0;
static uintptr_t g_pcmbufr = 0;
static unsigned int g_bufpos = 0;

static int16_t g_left[SJPCM_CHUNK_FRAMES] __attribute__((aligned(64)));
static int16_t g_right[SJPCM_CHUNK_FRAMES] __attribute__((aligned(64)));

static void ps2_audio_spin_delay(void)
{
    volatile int i;
    for (i = 0; i < 200000; i++)
        __asm__ __volatile__("" ::: "memory");
}

static void ps2_audio_wait_loops(int loops)
{
    int j;
    for (j = 0; j < loops; j++)
        ps2_audio_spin_delay();
}

static int ps2_audio_bind_rpc(void)
{
    int i;
    int ret;

    memset(&g_cd, 0, sizeof(g_cd));

    for (i = 0; i < 300; i++) {
        if ((i % 32) == 0) {
            SifInitRpc(0);
            memset(&g_cd, 0, sizeof(g_cd));
            printf("[SJPCM-EE] bind reinit try=%d\n", i);
        }

        ret = SifBindRpc(&g_cd, SJPCM_IRX, 0);

        if (g_cd.server) {
            printf("[SJPCM-EE] bind ok try=%d ret=%d server=%p\n", i, ret, g_cd.server);
            return 1;
        }

        if ((i % 20) == 0) {
            int found = SifSearchModuleByName("sjpcm");
            printf("[SJPCM-EE] bind wait try=%d ret=%d server=%p mod=%d\n",
                   i, ret, g_cd.server, found);
        }

        ps2_audio_spin_delay();
    }

    printf("[SJPCM-EE] bind timeout/fail server=%p\n", g_cd.server);
    return 0;
}

static int ps2_audio_rpc_simple(int fno, int send_bytes, int recv_bytes)
{
    return SifCallRpc(&g_cd, fno, 0,
        (void *)&g_sbuff[0], send_bytes,
        (void *)&g_sbuff[0], recv_bytes,
        0, 0);
}

int ps2_audio_init_once(void)
{
    int r;

    if (g_audio_state != 0) {
        printf("[SJPCM-EE] init skipped state=%d\n", g_audio_state);
        return g_audio_state > 0;
    }

    printf("[SJPCM-EE] init enter (IRX exec path)\n");

    SifInitRpc(0);
    SifLoadFileInit();
    printf("[SJPCM-EE] using prepatched boot environment\n");
    printf("[SJPCM-EE] before exec\n");

    printf("[SJPCM-EE] embedded irx size=%u bytes\n", (unsigned)libsdpcm_irx_len);

    {
        void *iop_mod = NULL;
        struct t_SifDmaTransfer dmat;
        int dma_id = -1;
        int mod_res = 0;

        iop_mod = SifAllocIopHeap((int)libsdpcm_irx_len);
        printf("[SJPCM-EE] SifAllocIopHeap -> %p size=%u\n", iop_mod, (unsigned)libsdpcm_irx_len);
        if (!iop_mod) {
            g_audio_state = -1;
            printf("[SJPCM-EE] init FAIL at SifAllocIopHeap\n");
            return 0;
        }

        memset(&dmat, 0, sizeof(dmat));
        dmat.src  = (void *)libsdpcm_irx;
        dmat.dest = iop_mod;
        dmat.size = (unsigned int)libsdpcm_irx_len;
        dmat.attr = 0;

        FlushCache(0);
        dma_id = SifSetDma(&dmat, 1);
        printf("[SJPCM-EE] SifSetDma(blob->iop) -> %d dest=%p size=%u\n",
               dma_id, iop_mod, (unsigned)libsdpcm_irx_len);
        if (dma_id < 0) {
            g_audio_state = -1;
            printf("[SJPCM-EE] init FAIL at SifSetDma\n");
            return 0;
        }

        while (SifDmaStat(dma_id) >= 0) { }

        r = SifLoadStartModuleBuffer(iop_mod, 0, NULL, &mod_res);
        printf("[SJPCM-EE] SifLoadStartModuleBuffer -> mod_id=%d mod_res=%d iop=%p\n",
               r, mod_res, iop_mod);
        if (r < 0) {
            g_audio_state = -1;
            printf("[SJPCM-EE] init FAIL at SifLoadStartModuleBuffer\n");
            return 0;
        }
    }

    SifInitRpc(0);
    SifLoadFileInit();
    printf("[SJPCM-EE] after reinit post-exec\n");

    ps2_audio_wait_loops(40);

    printf("[SJPCM-EE] SifSearchModuleByName(sjpcm) immediate -> %d\n", SifSearchModuleByName("sjpcm"));

    if (!ps2_audio_bind_rpc()) {
        g_audio_state = -1;
        printf("[SJPCM-EE] init FAIL at bind\n");
        return 0;
    }

    memset(&g_sbuff[0], 0, sizeof(g_sbuff));
    g_sbuff[0] = 0;

    r = ps2_audio_rpc_simple(SJPCM_INIT, 64, 64);
    printf("[SJPCM-EE] SJPCM_INIT ret=%d sbuf1=%u sbuf2=%u sbuf3=%u\n", r, g_sbuff[1], g_sbuff[2], g_sbuff[3]);
    if (r < 0) {
        g_audio_state = -1;
        printf("[SJPCM-EE] init FAIL at SJPCM_INIT\n");
        return 0;
    }

    FlushCache(0);

    g_pcmbufl = (uintptr_t)g_sbuff[1];
    g_pcmbufr = (uintptr_t)g_sbuff[2];
    g_bufpos  = g_sbuff[3];

    r = ps2_audio_rpc_simple(SJPCM_PLAY, 0, 0);
    printf("[SJPCM-EE] SJPCM_PLAY ret=%d\n", r);
    if (r < 0) {
        g_audio_state = -1;
        printf("[SJPCM-EE] init FAIL at SJPCM_PLAY\n");
        return 0;
    }

    g_audio_state = 1;
    printf("[SJPCM-EE] init success\n");
    return 1;
}

void ps2_audio_shutdown(void)
{
    if (g_audio_state == 1) {
        memset(&g_sbuff[0], 0, sizeof(g_sbuff));
        printf("[SJPCM-EE] shutdown -> SJPCM_QUIT\n");
        ps2_audio_rpc_simple(SJPCM_QUIT, 0, 0);
    }

    g_audio_state = 0;
}

size_t ps2_audio_push_samples(const int16_t *data, size_t frames)
{
    size_t done = 0;
    static int warned_not_ready = 0;

    if (g_audio_state != 1 || !g_cd.server) {
        if (!warned_not_ready) {
            printf("[SJPCM-EE] drop push: audio not ready state=%d server=%p\n", g_audio_state, g_cd.server);
            warned_not_ready = 1;
        }
        return frames;
    }

    warned_not_ready = 0;

    while (done < frames) {
        unsigned int chunk = (unsigned int)(frames - done);
        struct t_SifDmaTransfer sdt;
        int dma_id;
        unsigned int i;

        if (chunk > SJPCM_CHUNK_FRAMES)
            chunk = SJPCM_CHUNK_FRAMES;

        for (i = 0; i < chunk; i++) {
            g_left[i]  = data[(done + i) * 2 + 0];
            g_right[i] = data[(done + i) * 2 + 1];
        }

        sdt.src  = (void *)g_left;
        sdt.dest = (void *)(g_pcmbufl + g_bufpos);
        sdt.size = chunk * 2;
        sdt.attr = 0;

        FlushCache(0);
        dma_id = SifSetDma(&sdt, 1);
        while (SifDmaStat(dma_id) >= 0) { }

        sdt.src  = (void *)g_right;
        sdt.dest = (void *)(g_pcmbufr + g_bufpos);
        sdt.size = chunk * 2;
        sdt.attr = 0;

        FlushCache(0);
        dma_id = SifSetDma(&sdt, 1);
        while (SifDmaStat(dma_id) >= 0) { }

        memset(&g_sbuff[0], 0, sizeof(g_sbuff));
        g_sbuff[0] = chunk;

        if (ps2_audio_rpc_simple(SJPCM_ENQUEUE, 64, 64) < 0) {
            printf("[SJPCM-EE] SJPCM_ENQUEUE FAIL\n");
            break;
        }

        g_bufpos = g_sbuff[3];
        done += chunk;
    }

    return frames;
}
