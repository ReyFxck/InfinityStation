#include "ps2_audio.h"

#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <sifcmd.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <iopheap.h>
#include <sbv_patches.h>
#include <audsrv.h>
#include "audsrv_irx_blob.h"

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* QUIET_PS2AUDIO_LOGS_BEGIN */
#define QUIET_PS2AUDIO_LOGS 1
#if QUIET_PS2AUDIO_LOGS
#define PS2AUDIO_LOG(...) ((void)0)
#else
#define PS2AUDIO_LOG(...) printf(__VA_ARGS__)
#endif
/* QUIET_PS2AUDIO_LOGS_END */


/*
 * Estrutura:
 * - boot/init do PS2 no estilo PicoDrive
 * - ringbuffer + thread de audio no EE
 * - backend abstrato (sem audsrv/sjpcm aqui)
 *
 * Trocar depois só estas funcs:
 *   ps2_backend_init()
 *   ps2_backend_queue_audio()
 *   ps2_backend_queued_bytes()
 *   ps2_backend_shutdown()
 */

#define PS2_AUDIO_RATE              44100
#define CORE_AUDIO_RATE             32040
#define RESAMPLE_OUT_MAX_FRAMES     1024
#define PS2_AUDIO_CHANNELS          2
#define PS2_AUDIO_BYTES_PER_SAMPLE  2
#define PS2_AUDIO_FRAME_BYTES       (PS2_AUDIO_CHANNELS * PS2_AUDIO_BYTES_PER_SAMPLE)

/* Quantos blocos ficam no ringbuffer */
#define SOUND_BLOCK_COUNT           8

/* Frames por bloco */
#define SOUND_BUFFER_CHUNK          1024

/* Tamanho total do buffer em frames */
#define SOUND_TOTAL_FRAMES          (SOUND_BLOCK_COUNT * SOUND_BUFFER_CHUNK)

/* Tamanho total do buffer em bytes */
#define SOUND_TOTAL_BYTES           (SOUND_TOTAL_FRAMES * PS2_AUDIO_FRAME_BYTES)

/* Quanto o backend deve tentar consumir por vez */
#define BACKEND_FEED_FRAMES         256
#define BACKEND_FEED_BYTES          (BACKEND_FEED_FRAMES * PS2_AUDIO_FRAME_BYTES)

/* Thread priority */
#define SOUND_THREAD_PRIO           96
#define SOUND_THREAD_STACK          0x2000

/* ===================================================================== */
/* Estado geral                                                           */
/* ===================================================================== */

static int g_audio_state = 0; /* 0=off, 1=ready, -1=failed */
static int g_warned_not_ready = 0;
static int g_warned_overrun = 0;
static int g_warned_underrun = 0;
static int g_logged_first_push = 0;
static int g_audsrv_loaded = 0;

static int g_sound_tid = -1;
static volatile int g_sound_thread_running = 0;
static volatile int g_sound_thread_exit = 0;

/* stack real da thread de audio */
static uint8_t g_sound_thread_stack[SOUND_THREAD_STACK] __attribute__((aligned(64)));

/* Ringbuffer PCM estéreo 16-bit */
static int16_t g_ringbuf[SOUND_TOTAL_FRAMES * PS2_AUDIO_CHANNELS] __attribute__((aligned(64)));

/*
 * Índices em frames.
 * write_pos: onde o emulador escreve
 * read_pos : onde a thread entrega ao backend
 * buffered : quantos frames válidos existem
 */
static volatile unsigned int g_write_pos = 0;
static volatile unsigned int g_read_pos = 0;
static volatile unsigned int g_buffered_frames = 0;

/* Scratch block contínuo para alimentar backend */
static int16_t g_backend_block[BACKEND_FEED_FRAMES * PS2_AUDIO_CHANNELS] __attribute__((aligned(64)));
static int16_t g_resample_out[RESAMPLE_OUT_MAX_FRAMES * PS2_AUDIO_CHANNELS] __attribute__((aligned(64)));
static unsigned int g_resample_phase = 0;
static int16_t g_resample_prev_l = 0;
static int16_t g_resample_prev_r = 0;
static int g_resample_have_prev = 0;

/* ===================================================================== */
/* Utilidades                                                             */
/* ===================================================================== */

static void ps2_audio_wait_loops(int loops)
{
    volatile int i;
    while (loops-- > 0) {
        for (i = 0; i < 0x1000; i++) {
        }
        RotateThreadReadyQueue(SOUND_THREAD_PRIO);
    }
}

static void ps2_audio_log_state(const char *tag)
{
    PS2AUDIO_LOG("[PS2AUDIO] %s wp=%u rp=%u buffered=%u\n",
           tag,
           g_write_pos,
           g_read_pos,
           g_buffered_frames);
}

static void ps2_audio_clear_ring(void)
{
    memset(g_ringbuf, 0, sizeof(g_ringbuf));
    g_write_pos = 0;
    g_read_pos = 0;
    g_buffered_frames = 0;
}

static unsigned int ps2_audio_ring_free_frames(void)
{
    return SOUND_TOTAL_FRAMES - g_buffered_frames;
}

static unsigned int ps2_audio_ring_buffered_frames(void)
{
    return g_buffered_frames;
}

/* ===================================================================== */
/* Boot/init do PS2 estilo PicoDrive                                      */
/* ===================================================================== */

static void ps2_audio_reset_iop(void)
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

    PS2AUDIO_LOG("[PS2AUDIO] reset IOP done\n");
}

/* ===================================================================== */
/* Backend abstrato                                                       */
/* ===================================================================== */

/*
 * IMPLEMENTAR DEPOIS:
 *
 * Retorno esperado:
 *   0  = ok
 *  <0  = falha
 *
 * queued_bytes:
 *   quantos bytes ainda estão pendentes no backend
 *
 * queue_audio:
 *   envia "bytes" do buffer PCM estéreo s16le
 *   retorna quantidade aceita:
 *      bytes  = aceitou tudo
 *      >= 0   = aceitou parcial
 *      < 0    = erro
 */


static int ps2_backend_init(int rate, int channels, int bits)
{
    audsrv_fmt_t fmt;
    int ret;
    int mod;

    PS2AUDIO_LOG("[PS2AUDIO] backend_init audsrv rate=%d ch=%d bits=%d\n", rate, channels, bits);

    mod = SifLoadModule("rom0:LIBSD", 0, NULL);
    PS2AUDIO_LOG("[PS2AUDIO] load LIBSD for audsrv -> %d\n", mod);
    if (mod < 0)
        return -1;

    {
        const unsigned char *pp = (const unsigned char *)audsrv_irx;
        PS2AUDIO_LOG("[PS2AUDIO] audsrv irx first16: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
            pp[0], pp[1], pp[2], pp[3], pp[4], pp[5], pp[6], pp[7],
            pp[8], pp[9], pp[10], pp[11], pp[12], pp[13], pp[14], pp[15]);
    }

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

static int ps2_backend_queued_bytes(void)
{
    int q = audsrv_queued();
    if (q < 0)
        return 0;
    return q;
}

static int ps2_backend_available_bytes(void)
{
    int a = audsrv_available();
    if (a < 0)
        return 0;
    return a;
}

static int ps2_backend_queue_audio(const int16_t *data, int bytes)
{
    int ret = audsrv_play_audio((char *)data, bytes);
    if (ret < 0) {
        PS2AUDIO_LOG("[PS2AUDIO] audsrv_play_audio FAIL -> %d\n", ret);
        return ret;
    }
    return bytes;
}

static void ps2_backend_shutdown(void)
{
    if (g_audsrv_loaded) {
        PS2AUDIO_LOG("[PS2AUDIO] audsrv_shutdown\n");
        audsrv_quit();
        g_audsrv_loaded = 0;
    }
}

/* ===================================================================== */
/* Ringbuffer                                                             */
/* ===================================================================== */

static unsigned int ps2_audio_copy_from_ring(int16_t *dst, unsigned int frames)
{
    unsigned int copied = 0;

    while (copied < frames && g_buffered_frames > 0) {
        unsigned int rp = g_read_pos;
        unsigned int avail_until_wrap = SOUND_TOTAL_FRAMES - rp;
        unsigned int take = frames - copied;

        if (take > g_buffered_frames)
            take = g_buffered_frames;
        if (take > avail_until_wrap)
            take = avail_until_wrap;

        memcpy(&dst[copied * PS2_AUDIO_CHANNELS],
               &g_ringbuf[rp * PS2_AUDIO_CHANNELS],
               take * PS2_AUDIO_FRAME_BYTES);

        g_read_pos = (g_read_pos + take) % SOUND_TOTAL_FRAMES;
        g_buffered_frames -= take;
        copied += take;
    }

    return copied;
}

static unsigned int ps2_audio_copy_to_ring(const int16_t *src, unsigned int frames)
{
    unsigned int written = 0;

    while (written < frames) {
        unsigned int free_frames = ps2_audio_ring_free_frames();
        unsigned int wp = g_write_pos;
        unsigned int avail_until_wrap = SOUND_TOTAL_FRAMES - wp;
        unsigned int put = frames - written;

        if (free_frames == 0)
            break;
        if (put > free_frames)
            put = free_frames;
        if (put > avail_until_wrap)
            put = avail_until_wrap;

        memcpy(&g_ringbuf[wp * PS2_AUDIO_CHANNELS],
               &src[written * PS2_AUDIO_CHANNELS],
               put * PS2_AUDIO_FRAME_BYTES);

        g_write_pos = (g_write_pos + put) % SOUND_TOTAL_FRAMES;
        g_buffered_frames += put;
        written += put;
    }

    return written;
}

/* ===================================================================== */
/* Thread de áudio                                                        */
/* ===================================================================== */

static void ps2_audio_sound_thread(void *arg)
{
    (void)arg;

    g_sound_thread_running = 1;
    PS2AUDIO_LOG("[PS2AUDIO] sound thread start\n");

    while (!g_sound_thread_exit) {
        int queued_bytes;
        unsigned int buffered_frames;
        unsigned int want_frames;
        unsigned int got_frames;
        int sent_bytes;

        if (g_audio_state != 1) {
            ps2_audio_wait_loops(1);
            continue;
        }

        queued_bytes = ps2_backend_queued_bytes();
        buffered_frames = ps2_audio_ring_buffered_frames();

        if (buffered_frames == 0) {
            if (!g_warned_underrun) {
                PS2AUDIO_LOG("[PS2AUDIO] underrun: ring empty\n");
                g_warned_underrun = 1;
            }
            ps2_audio_wait_loops(1);
            continue;
        }

        g_warned_underrun = 0;

        /*
         * Backend já cheio demais? espera um pouco.
         * O valor exato pode ser ajustado quando o backend real existir.
         */
        if (queued_bytes > (BACKEND_FEED_BYTES * 3)) {
            ps2_audio_wait_loops(1);
            continue;
        }

        want_frames = BACKEND_FEED_FRAMES;
        if (want_frames > buffered_frames)
            want_frames = buffered_frames;

        got_frames = ps2_audio_copy_from_ring(g_backend_block, want_frames);
        if (got_frames == 0) {
            ps2_audio_wait_loops(1);
            continue;
        }

        FlushCache(0);

        sent_bytes = ps2_backend_queue_audio(g_backend_block, (int)(got_frames * PS2_AUDIO_FRAME_BYTES));
        if (sent_bytes < 0) {
            PS2AUDIO_LOG("[PS2AUDIO] backend_queue_audio FAIL\n");
            ps2_audio_wait_loops(4);
            continue;
        }

        if ((unsigned int)sent_bytes != got_frames * PS2_AUDIO_FRAME_BYTES) {
            PS2AUDIO_LOG("[PS2AUDIO] backend accepted partial=%d expected=%u\n",
                   sent_bytes, got_frames * PS2_AUDIO_FRAME_BYTES);
        }
    }

    g_sound_thread_running = 0;
    PS2AUDIO_LOG("[PS2AUDIO] sound thread end\n");
    ExitDeleteThread();
}

static int ps2_audio_start_thread(void)
{
    int ret;
    ee_thread_t thparam;

    memset(&thparam, 0, sizeof(thparam));
    thparam.func = ps2_audio_sound_thread;
    thparam.stack = g_sound_thread_stack;
    thparam.stack_size = SOUND_THREAD_STACK;
    thparam.gp_reg = &_gp;
    thparam.initial_priority = SOUND_THREAD_PRIO;

    g_sound_tid = CreateThread(&thparam);
    if (g_sound_tid < 0) {
        PS2AUDIO_LOG("[PS2AUDIO] CreateThread FAIL -> %d\n", g_sound_tid);
        g_sound_tid = -1;
        return -1;
    }

    g_sound_thread_exit = 0;

    ret = StartThread(g_sound_tid, NULL);
    if (ret < 0) {
        PS2AUDIO_LOG("[PS2AUDIO] StartThread FAIL -> %d\n", ret);
        DeleteThread(g_sound_tid);
        g_sound_tid = -1;
        return -1;
    }

    return 0;
}

static void ps2_audio_stop_thread(void)
{
    int spins = 0;

    if (g_sound_tid < 0)
        return;

    g_sound_thread_exit = 1;

    while (g_sound_thread_running && spins < 4000) {
        ps2_audio_wait_loops(1);
        spins++;
    }

    if (g_sound_thread_running) {
        PS2AUDIO_LOG("[PS2AUDIO] thread did not exit cleanly, terminating\n");
        TerminateThread(g_sound_tid);
        DeleteThread(g_sound_tid);
    }

    g_sound_tid = -1;
    g_sound_thread_running = 0;
}

/* ===================================================================== */
/* API pública                                                            */
/* ===================================================================== */

int ps2_audio_init_once(void)
{
    int ret;

    if (g_audio_state != 0) {
        PS2AUDIO_LOG("[PS2AUDIO] init skipped state=%d\n", g_audio_state);
        return g_audio_state > 0;
    }

    PS2AUDIO_LOG("[PS2AUDIO] init enter (PicoDrive-style skeleton)\n");

    ps2_audio_reset_iop();
    ps2_audio_clear_ring();

    ret = ps2_backend_init(PS2_AUDIO_RATE, PS2_AUDIO_CHANNELS, 16);
    if (ret < 0) {
        g_audio_state = -1;
        PS2AUDIO_LOG("[PS2AUDIO] FAIL: backend_init\n");
        return 0;
    }

    PS2AUDIO_LOG("[PS2AUDIO] thread bypass debug: not starting sound thread\n");

    g_warned_not_ready = 0;
    g_warned_overrun = 0;
    g_warned_underrun = 0;
    g_audio_state = 1;

    ps2_audio_log_state("init success");
    return 1;
}


void ps2_audio_pump(void)
{
    /* direct-audsrv debug path: sem pump/ringbuffer */
}

void ps2_audio_shutdown(void)
{
    if (g_audio_state == 1) {
        PS2AUDIO_LOG("[PS2AUDIO] shutdown begin\n");
    }

    ps2_audio_stop_thread();
    ps2_backend_shutdown();
    ps2_audio_clear_ring();

    g_audio_state = 0;
    g_warned_not_ready = 0;
    g_warned_overrun = 0;
    g_warned_underrun = 0;

    PS2AUDIO_LOG("[PS2AUDIO] shutdown done\n");
}

/*
 * Recebe PCM estéreo s16 interleaved:
 * L R L R ...
 *
 * Retorna quantos frames foram aceitos.
 */
size_t ps2_audio_push_samples(const int16_t *data, size_t frames)
{
    size_t done = 0;

    if (g_audio_state != 1) {
        if (!g_warned_not_ready) {
            PS2AUDIO_LOG("[PS2AUDIO] drop push: audio not ready state=%d\n", g_audio_state);
            g_warned_not_ready = 1;
        }
        return frames;
    }

    g_warned_not_ready = 0;

    if (!g_logged_first_push) {
        PS2AUDIO_LOG("[PS2AUDIO] first push frames=%u\n", (unsigned)frames);
        g_logged_first_push = 1;
    }

    while (done < frames) {
        size_t chunk = frames - done;
        size_t out_frames = 0;
        size_t i;
        int bytes;
        int ret;

        if (chunk > BACKEND_FEED_FRAMES)
            chunk = BACKEND_FEED_FRAMES;

        if (!g_resample_have_prev && chunk > 0) {
            g_resample_prev_l = data[done * 2 + 0];
            g_resample_prev_r = data[done * 2 + 1];
            g_resample_have_prev = 1;
        }

        for (i = 0; i < chunk && out_frames < RESAMPLE_OUT_MAX_FRAMES; i++) {
            int16_t cur_l = data[(done + i) * 2 + 0];
            int16_t cur_r = data[(done + i) * 2 + 1];

            while (g_resample_phase < PS2_AUDIO_RATE && out_frames < RESAMPLE_OUT_MAX_FRAMES) {
                int a = (int)g_resample_phase;
                int b = (int)(PS2_AUDIO_RATE - a);

                g_resample_out[out_frames * 2 + 0] =
                    (int16_t)(((int)g_resample_prev_l * b + (int)cur_l * a) / (int)PS2_AUDIO_RATE);
                g_resample_out[out_frames * 2 + 1] =
                    (int16_t)(((int)g_resample_prev_r * b + (int)cur_r * a) / (int)PS2_AUDIO_RATE);
                out_frames++;
                g_resample_phase += CORE_AUDIO_RATE;
            }

            while (g_resample_phase >= PS2_AUDIO_RATE)
                g_resample_phase -= PS2_AUDIO_RATE;

            g_resample_prev_l = cur_l;
            g_resample_prev_r = cur_r;
        }

        if (out_frames == 0) {
            done += chunk;
            continue;
        }

        bytes = (int)(out_frames * PS2_AUDIO_FRAME_BYTES);

        audsrv_wait_audio(bytes);
        ret = ps2_backend_queue_audio(g_resample_out, bytes);
        if (ret < 0) {
            PS2AUDIO_LOG("[PS2AUDIO] direct audsrv queue FAIL -> %d\n", ret);
            break;
        }

        done += chunk;
    }

    return done;
}



