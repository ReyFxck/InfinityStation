#include "ps2_audio_internal.h"

#include <tamtypes.h>
#include <kernel.h>
#include <delaythread.h>
#include <sifrpc.h>
#include <sifcmd.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <iopheap.h>
#include <sbv_patches.h>

#include <string.h>

#define PS2_AUDIO_WAIT_SLICE_US 250

static int g_audio_ring_sema = -1;

static int g_audio_state = 0; /* 0=off, 1=ready, -1=failed */
static int g_warned_not_ready = 0;
static int g_warned_overrun = 0;
static int g_warned_underrun = 0;
static int g_logged_first_push = 0;
static int g_audio_paused = 0;
static int g_audio_resume_armed = 0;

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
static int16_t g_menu_silence_block[BACKEND_FEED_FRAMES * PS2_AUDIO_CHANNELS] __attribute__((aligned(64)));

static void ps2_audio_ring_lock(void)
{
    if (g_audio_ring_sema >= 0)
        WaitSema(g_audio_ring_sema);
}

static void ps2_audio_ring_unlock(void)
{
    if (g_audio_ring_sema >= 0)
        SignalSema(g_audio_ring_sema);
}

static void ps2_audio_wait_loops(int loops)
{
    int sleep_us;

    if (loops <= 0)
        return;

    sleep_us = loops * PS2_AUDIO_WAIT_SLICE_US;
    if (sleep_us < PS2_AUDIO_WAIT_SLICE_US)
        sleep_us = PS2_AUDIO_WAIT_SLICE_US;

    if (DelayThread(sleep_us) < 0)
        RotateThreadReadyQueue(SOUND_THREAD_PRIO);
}

static void ps2_audio_reset_stream_state(void)
{
    g_resample_phase = 0;
    g_resample_prev_l = 0;
    g_resample_prev_r = 0;
    g_resample_have_prev = 0;
    memset(g_resample_out, 0, sizeof(g_resample_out));
}

static void ps2_audio_log_state(const char *tag)
{
    PS2AUDIO_LOG("[PS2AUDIO] %s wp=%u rp=%u buffered=%u paused=%d\n",
           tag,
           g_write_pos,
           g_read_pos,
           g_buffered_frames,
           g_audio_paused);
}

static void ps2_audio_clear_ring(void)
{
    ps2_audio_ring_lock();
    memset(g_ringbuf, 0, sizeof(g_ringbuf));
    g_write_pos = 0;
    g_read_pos = 0;
    g_buffered_frames = 0;
    ps2_audio_ring_unlock();
}

static unsigned int ps2_audio_ring_free_frames(void)
{
    return SOUND_TOTAL_FRAMES - g_buffered_frames;
}

static unsigned int ps2_audio_ring_buffered_frames(void)
{
    unsigned int v;
    ps2_audio_ring_lock();
    v = g_buffered_frames;
    ps2_audio_ring_unlock();
    return v;
}

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

static unsigned int ps2_audio_copy_from_ring(int16_t *dst, unsigned int frames)
{
    unsigned int copied = 0;

    ps2_audio_ring_lock();

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

    ps2_audio_ring_unlock();
    return copied;
}

static unsigned int ps2_audio_copy_to_ring(const int16_t *src, unsigned int frames)
{
    unsigned int written = 0;

    ps2_audio_ring_lock();

    while (written < frames) {
        unsigned int free_frames = SOUND_TOTAL_FRAMES - g_buffered_frames;
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

    ps2_audio_ring_unlock();
    return written;
}

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

        if (queued_bytes > (BACKEND_FEED_BYTES * 4)) {
            ps2_audio_wait_loops(1);
            continue;
        }

        if (g_audio_paused || buffered_frames == 0) {
            if (queued_bytes <= BACKEND_FEED_BYTES) {
                if (!g_audio_paused && !g_warned_underrun) {
                    PS2AUDIO_LOG("[PS2AUDIO] underrun: feeding silence\n");
                    g_warned_underrun = 1;
                }
                ps2_backend_wait_audio(BACKEND_FEED_BYTES);
                FlushCache(0);
                (void)ps2_backend_queue_audio(g_menu_silence_block, BACKEND_FEED_BYTES);
            }
            ps2_audio_wait_loops(1);
            continue;
        }

        g_warned_underrun = 0;

        want_frames = BACKEND_FEED_FRAMES;
        if (want_frames > buffered_frames)
            want_frames = buffered_frames;

        got_frames = ps2_audio_copy_from_ring(g_backend_block, want_frames);
        if (got_frames == 0) {
            ps2_audio_wait_loops(1);
            continue;
        }

        ps2_backend_wait_audio((int)(got_frames * PS2_AUDIO_FRAME_BYTES));
        FlushCache(0);

        sent_bytes = ps2_backend_queue_audio(
            g_backend_block,
            (int)(got_frames * PS2_AUDIO_FRAME_BYTES)
        );
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

int ps2_audio_init_once(void)
{
    int ret;

    if (g_audio_state != 0) {
        PS2AUDIO_LOG("[PS2AUDIO] init skipped state=%d\n", g_audio_state);
        return g_audio_state > 0;
    }

    PS2AUDIO_LOG("[PS2AUDIO] init enter (phase 1 split)\n");

    ps2_audio_reset_iop();

    if (g_audio_ring_sema < 0) {
        ee_sema_t sema;
        memset(&sema, 0, sizeof(sema));
        sema.init_count = 1;
        sema.max_count = 1;
        g_audio_ring_sema = CreateSema(&sema);
        if (g_audio_ring_sema < 0) {
            g_audio_state = -1;
            PS2AUDIO_LOG("[PS2AUDIO] FAIL: CreateSema ring\n");
            return 0;
        }
    }

    ps2_audio_clear_ring();
    ps2_audio_reset_stream_state();
    memset(g_menu_silence_block, 0, sizeof(g_menu_silence_block));
    g_audio_paused = 0;
    g_audio_resume_armed = 0;

    ret = ps2_backend_init(PS2_AUDIO_RATE, PS2_AUDIO_CHANNELS, 16);
    if (ret < 0) {
        g_audio_state = -1;
        PS2AUDIO_LOG("[PS2AUDIO] FAIL: backend_init\n");
        return 0;
    }

    ret = ps2_audio_start_thread();
    if (ret < 0) {
        g_audio_state = -1;
        ps2_backend_shutdown();
        PS2AUDIO_LOG("[PS2AUDIO] FAIL: start sound thread\n");
        return 0;
    }

    g_warned_not_ready = 0;
    g_warned_overrun = 0;
    g_warned_underrun = 0;
    g_audio_state = 1;
    ps2_backend_set_volume(0x3fff);

    ps2_audio_log_state("init success");
    return 1;
}

void ps2_audio_pump(void)
{
    if (g_audio_state != 1 || !g_audio_paused)
        return;

    if (g_sound_thread_running)
        return;

    ps2_backend_wait_audio(BACKEND_FEED_BYTES);
    (void)ps2_backend_queue_audio(g_menu_silence_block, BACKEND_FEED_BYTES);
}

void ps2_audio_pause(void)
{
    if (g_audio_state != 1)
        return;

    g_audio_paused = 1;
    g_audio_resume_armed = 1;

    /* pause suave estilo PGEN: backend continua vivo, mas mutado */
    ps2_backend_set_volume(0);

    ps2_audio_clear_ring();
    ps2_audio_reset_stream_state();
    memset(g_menu_silence_block, 0, sizeof(g_menu_silence_block));

    g_warned_overrun = 0;
    g_warned_underrun = 0;
    g_logged_first_push = 0;
    ps2_audio_log_state("paused");
}

void ps2_audio_resume(void)
{
    if (g_audio_state != 1)
        return;

    /* continua mutado ate o primeiro audio real do jogo chegar */
    g_audio_paused = 0;
    g_audio_resume_armed = 1;
    ps2_audio_clear_ring();
    ps2_audio_reset_stream_state();
    g_warned_not_ready = 0;
    g_warned_overrun = 0;
    g_warned_underrun = 0;
    g_logged_first_push = 0;
    ps2_audio_log_state("resumed-armed");
}

void ps2_audio_shutdown(void)
{
    if (g_audio_state == 1) {
        PS2AUDIO_LOG("[PS2AUDIO] shutdown begin\n");
    }

    g_audio_paused = 1;
    g_audio_resume_armed = 0;
    ps2_backend_set_volume(0);
    ps2_audio_stop_thread();
    ps2_backend_shutdown();
    ps2_audio_clear_ring();
    ps2_audio_reset_stream_state();

    if (g_audio_ring_sema >= 0) {
        DeleteSema(g_audio_ring_sema);
        g_audio_ring_sema = -1;
    }

    g_audio_state = 0;
    g_warned_not_ready = 0;
    g_warned_overrun = 0;
    g_warned_underrun = 0;
    g_logged_first_push = 0;

    PS2AUDIO_LOG("[PS2AUDIO] shutdown done\n");
}

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

    if (g_audio_paused)
        return frames;

    if (g_audio_resume_armed && ps2_audio_ring_buffered_frames() >= (BACKEND_FEED_FRAMES * 2)) {
        ps2_backend_set_volume(0x3fff);
        g_audio_resume_armed = 0;
    }

    if (!g_logged_first_push) {
        PS2AUDIO_LOG("[PS2AUDIO] first push frames=%u\n", (unsigned)frames);
        g_logged_first_push = 1;
    }

    while (done < frames) {
        size_t chunk = frames - done;
        size_t out_frames;
        unsigned int written;

        if (chunk > BACKEND_FEED_FRAMES)
            chunk = BACKEND_FEED_FRAMES;

        out_frames = ps2_audio_resample_chunk(&data[done * 2], chunk);
        if (out_frames == 0) {
            done += chunk;
            continue;
        }

        if (!g_sound_thread_running) {
            int bytes = (int)(out_frames * PS2_AUDIO_FRAME_BYTES);
            ps2_backend_wait_audio(bytes);
            (void)ps2_backend_queue_audio(g_resample_out, bytes);
            g_warned_overrun = 0;
            done += chunk;
            continue;
        }

        written = ps2_audio_copy_to_ring(g_resample_out, (unsigned int)out_frames);
        if (written < out_frames) {
            if (!g_warned_overrun) {
                PS2AUDIO_LOG("[PS2AUDIO] ring overrun: written=%u expected=%u\n",
                       written, (unsigned int)out_frames);
                g_warned_overrun = 1;
            }

            if (written == 0) {
                int bytes = (int)(out_frames * PS2_AUDIO_FRAME_BYTES);
                ps2_backend_wait_audio(bytes);
                (void)ps2_backend_queue_audio(g_resample_out, bytes);
            }
        } else {
            g_warned_overrun = 0;
        }

        done += chunk;
    }

    return done;
}
