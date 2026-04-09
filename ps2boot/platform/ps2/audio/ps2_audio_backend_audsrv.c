#include "ps2_audio_backend.h"
#include "ps2_audio_internal.h"

#include <sifrpc.h>
#include <sifcmd.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <iopheap.h>
#include <sbv_patches.h>
#include <audsrv.h>
#include "audsrv_irx_blob.h"

#include <string.h>
#include <kernel.h>

static int g_audsrv_loaded = 0;
static int g_iop_ready = 0;

void ps2_audio_backend_set_iop_ready(int ready)
{
    g_iop_ready = ready ? 1 : 0;
}

int ps2_audio_backend_iop_ready(void)
{
    return g_iop_ready;
}

int ps2_audio_backend_reset_iop(void)
{
    PS2AUDIO_LOG("[PS2AUDIO] reset IOP begin\n");

    while (!SifIopReset(NULL, 0)) {
    }
    while (!SifIopSync()) {
    }

    SifExitIopHeap();
    SifLoadFileExit();
    SifExitRpc();
    SifExitCmd();

    SifInitRpc(0);
    SifLoadFileInit();

    FlushCache(0);
    FlushCache(2);

    {
        int patch_lmb_ret = sbv_patch_enable_lmb();
        int patch_prefix_ret = sbv_patch_disable_prefix_check();
        (void)patch_lmb_ret;
        (void)patch_prefix_ret;
        PS2AUDIO_LOG("[PS2AUDIO] sbv_patch_enable_lmb -> %d\n", patch_lmb_ret);
        PS2AUDIO_LOG("[PS2AUDIO] sbv_patch_disable_prefix_check -> %d\n", patch_prefix_ret);
    }

    g_iop_ready = 1;
    PS2AUDIO_LOG("[PS2AUDIO] reset IOP done\n");
    return 1;
}

int ps2_backend_init(int rate, int channels, int bits)
{
    audsrv_fmt_t fmt;
    int ret;
    int mod;

    PS2AUDIO_LOG("[PS2AUDIO] backend_init audsrv rate=%d ch=%d bits=%d\n", rate, channels, bits);

    mod = SifLoadModule("rom0:LIBSD", 0, NULL);
    PS2AUDIO_LOG("[PS2AUDIO] load LIBSD for audsrv -> %d\n", mod);
    if (mod < 0)
        return -1;

    PS2AUDIO_LOG("[PS2AUDIO] audsrv irx first16: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
        ((const unsigned char *)audsrv_irx)[0], ((const unsigned char *)audsrv_irx)[1],
        ((const unsigned char *)audsrv_irx)[2], ((const unsigned char *)audsrv_irx)[3],
        ((const unsigned char *)audsrv_irx)[4], ((const unsigned char *)audsrv_irx)[5],
        ((const unsigned char *)audsrv_irx)[6], ((const unsigned char *)audsrv_irx)[7],
        ((const unsigned char *)audsrv_irx)[8], ((const unsigned char *)audsrv_irx)[9],
        ((const unsigned char *)audsrv_irx)[10], ((const unsigned char *)audsrv_irx)[11],
        ((const unsigned char *)audsrv_irx)[12], ((const unsigned char *)audsrv_irx)[13],
        ((const unsigned char *)audsrv_irx)[14], ((const unsigned char *)audsrv_irx)[15]);

    ret = SifExecModuleBuffer((void *)audsrv_irx, size_audsrv_irx, 0, NULL, NULL);
    PS2AUDIO_LOG("[PS2AUDIO] SifExecModuleBuffer(audsrv) -> %d\n", ret);
    if (ret < 0)
        return -2;

    g_audsrv_loaded = 1;

    ret = audsrv_init();
    PS2AUDIO_LOG("[PS2AUDIO] audsrv_init -> %d\n", ret);
    if (ret < 0)
        return -3;

    memset(&fmt, 0, sizeof(fmt));
    fmt.freq = rate;
    fmt.bits = bits;
    fmt.channels = channels;

    ret = audsrv_set_format(&fmt);
    PS2AUDIO_LOG("[PS2AUDIO] audsrv_set_format -> %d\n", ret);
    if (ret < 0)
        return -4;

    return 0;
}

int ps2_backend_queued_bytes(void)
{
    int q = audsrv_queued();
    if (q < 0)
        return 0;
    return q;
}

int ps2_backend_available_bytes(void)
{
    int a = audsrv_available();
    if (a < 0)
        return 0;
    return a;
}

void ps2_backend_wait_audio(int bytes)
{
    audsrv_wait_audio(bytes);
}

int ps2_backend_queue_audio(const int16_t *data, int bytes)
{
    int ret = audsrv_play_audio((char *)data, bytes);
    if (ret < 0) {
        PS2AUDIO_LOG("[PS2AUDIO] audsrv_play_audio FAIL -> %d\n", ret);
        return ret;
    }
    return bytes;
}

void ps2_backend_stop_audio(void)
{
    if (!g_audsrv_loaded)
        return;

    PS2AUDIO_LOG("[PS2AUDIO] audsrv_stop_audio\n");
    audsrv_stop_audio();
}

int ps2_backend_set_volume(int vol)
{
    if (!g_audsrv_loaded)
        return -1;

    return audsrv_set_volume(vol);
}

void ps2_backend_shutdown(void)
{
    if (g_audsrv_loaded) {
        PS2AUDIO_LOG("[PS2AUDIO] audsrv_shutdown\n");
        audsrv_quit();
        g_audsrv_loaded = 0;
    }

    g_iop_ready = 0;
}
