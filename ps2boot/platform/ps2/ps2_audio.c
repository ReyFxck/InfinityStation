#include "ps2_audio.h"

#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <sifcmd.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <iopheap.h>
#include <sbv_patches.h>

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

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
#define BACKEND_FEED_FRAMES         SOUND_BUFFER_CHUNK
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
    printf("[PS2AUDIO] %s wp=%u rp=%u buffered=%u\n",
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
    printf("[PS2AUDIO] reset IOP begin\n");

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

    printf("[PS2AUDIO] sbv_patch_enable_lmb -> %d\n", sbv_patch_enable_lmb());
    printf("[PS2AUDIO] sbv_patch_disable_prefix_check -> %d\n", sbv_patch_disable_prefix_check());

    printf("[PS2AUDIO] reset IOP done\n");
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
    printf("[PS2AUDIO] backend_init stub rate=%d ch=%d bits=%d\n", rate, channels, bits);
    return 0;
}

static int ps2_backend_queued_bytes(void)
{
    return 0;
}

static int ps2_backend_queue_audio(const int16_t *data, int bytes)
{
    (void)data;
    return bytes;
}

static void ps2_backend_shutdown(void)
{
    printf("[PS2AUDIO] backend_shutdown stub\n");
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
    printf("[PS2AUDIO] sound thread start\n");

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
                printf("[PS2AUDIO] underrun: ring empty\n");
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
            printf("[PS2AUDIO] backend_queue_audio FAIL\n");
            ps2_audio_wait_loops(4);
            continue;
        }

        if ((unsigned int)sent_bytes != got_frames * PS2_AUDIO_FRAME_BYTES) {
            printf("[PS2AUDIO] backend accepted partial=%d expected=%u\n",
                   sent_bytes, got_frames * PS2_AUDIO_FRAME_BYTES);
        }
    }

    g_sound_thread_running = 0;
    printf("[PS2AUDIO] sound thread end\n");
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
        printf("[PS2AUDIO] CreateThread FAIL -> %d\n", g_sound_tid);
        g_sound_tid = -1;
        return -1;
    }

    g_sound_thread_exit = 0;

    ret = StartThread(g_sound_tid, NULL);
    if (ret < 0) {
        printf("[PS2AUDIO] StartThread FAIL -> %d\n", ret);
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
        printf("[PS2AUDIO] thread did not exit cleanly, terminating\n");
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
        printf("[PS2AUDIO] init skipped state=%d\n", g_audio_state);
        return g_audio_state > 0;
    }

    printf("[PS2AUDIO] init enter (PicoDrive-style skeleton)\n");

    ps2_audio_reset_iop();
    ps2_audio_clear_ring();

    ret = ps2_backend_init(PS2_AUDIO_RATE, PS2_AUDIO_CHANNELS, 16);
    if (ret < 0) {
        g_audio_state = -1;
        printf("[PS2AUDIO] FAIL: backend_init\n");
        return 0;
    }

    printf("[PS2AUDIO] thread bypass debug: not starting sound thread\n");

    g_warned_not_ready = 0;
    g_warned_overrun = 0;
    g_warned_underrun = 0;
    g_audio_state = 1;

    ps2_audio_log_state("init success");
    return 1;
}

void ps2_audio_shutdown(void)
{
    if (g_audio_state == 1) {
        printf("[PS2AUDIO] shutdown begin\n");
    }

    ps2_audio_stop_thread();
    ps2_backend_shutdown();
    ps2_audio_clear_ring();

    g_audio_state = 0;
    g_warned_not_ready = 0;
    g_warned_overrun = 0;
    g_warned_underrun = 0;

    printf("[PS2AUDIO] shutdown done\n");
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
            printf("[PS2AUDIO] drop push: audio not ready state=%d\n", g_audio_state);
            g_warned_not_ready = 1;
        }
        return frames;
    }

    g_warned_not_ready = 0;

    if (!g_logged_first_push) {
        printf("[PS2AUDIO] first push frames=%u\n", (unsigned)frames);
        g_logged_first_push = 1;
    }

    while (done < frames) {
        unsigned int free_frames = ps2_audio_ring_free_frames();
        unsigned int chunk = (unsigned int)(frames - done);
        unsigned int written;

        if (free_frames == 0) {
            if (!g_warned_overrun) {
                printf("[PS2AUDIO] overrun: ring full\n");
                g_warned_overrun = 1;
            }
            break;
        }

        g_warned_overrun = 0;

        if (chunk > free_frames)
            chunk = free_frames;

        written = ps2_audio_copy_to_ring(&data[done * PS2_AUDIO_CHANNELS], chunk);
        if (written == 0)
            break;

        done += written;
    }

    return done;
}
