#include "ps2_audio_internal.h"
#include "ps2_audio_backend.h"

#include <tamtypes.h>
#include <kernel.h>
#include <delaythread.h>

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
static unsigned int g_audio_backend_volume = PS2_AUDIO_BACKEND_VOLUME;
static int g_backend_reprime_cooldown = 0;
static int g_backend_empty_streak = 0;

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
    /* Sem resample: core e backend usam a mesma taxa. */
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





static void ps2_audio_set_backend_volume(unsigned int vol)
{
    if (vol > PS2_AUDIO_BACKEND_VOLUME)
        vol = PS2_AUDIO_BACKEND_VOLUME;

    g_audio_backend_volume = vol;
    ps2_backend_set_volume((int)vol);
}

static void ps2_audio_fade_backend_volume(unsigned int from, unsigned int to)
{
    unsigned int i;
    int start = (from > PS2_AUDIO_BACKEND_VOLUME) ? (int)PS2_AUDIO_BACKEND_VOLUME : (int)from;
    int end = (to > PS2_AUDIO_BACKEND_VOLUME) ? (int)PS2_AUDIO_BACKEND_VOLUME : (int)to;

    if (PS2_AUDIO_FADE_STEPS <= 1 || start == end) {
        ps2_audio_set_backend_volume((unsigned int)end);
        return;
    }

    for (i = 1; i <= PS2_AUDIO_FADE_STEPS; i++) {
        int vol = start + (((end - start) * (int)i) / PS2_AUDIO_FADE_STEPS);

        if (vol < 0)
            vol = 0;

        ps2_audio_set_backend_volume((unsigned int)vol);

        if (i != PS2_AUDIO_FADE_STEPS)
            DelayThread(PS2_AUDIO_FADE_DELAY_US);
    }
}

static unsigned int ps2_audio_ring_buffered_frames(void)
{
    unsigned int v;
    ps2_audio_ring_lock();
    v = g_buffered_frames;
    ps2_audio_ring_unlock();
    return v;
}



static void ps2_audio_finish_resume_if_ready(void)
{
    unsigned int buffered;

    if (g_audio_state != 1 || g_audio_paused || !g_audio_resume_armed)
        return;

    buffered = ps2_audio_ring_buffered_frames();
    if (buffered < BACKEND_RESUME_UNMUTE_FRAMES)
        return;

    g_audio_resume_armed = 0;
    g_warned_underrun = 0;
    ps2_audio_fade_backend_volume(g_audio_backend_volume, PS2_AUDIO_BACKEND_VOLUME);
    PS2AUDIO_LOG("[PS2AUDIO] resume unmuted buffered=%u\n", buffered);
}


void ps2_audio_get_buffer_status(bool *active, unsigned *occupancy, bool *underrun_likely)
{
    unsigned int ring_frames = ps2_audio_ring_buffered_frames();
    unsigned int backend_frames = 0;
    unsigned int total_frames;
    unsigned int occ = 0;
    int queued_bytes = 0;

    if ((g_audio_state == 1) && !g_audio_paused) {
        queued_bytes = ps2_backend_queued_bytes();
        if (queued_bytes > 0)
            backend_frames = (unsigned int)queued_bytes / PS2_AUDIO_FRAME_BYTES;
    }

    total_frames = ring_frames + backend_frames;

    if (BACKEND_QUEUE_TARGET_FRAMES)
        occ = (total_frames * 100U) / BACKEND_QUEUE_TARGET_FRAMES;

    if (occ > 100U)
        occ = 100U;

    if (active)
        *active = (g_audio_state == 1) && !g_audio_paused;

    if (occupancy)
        *occupancy = occ;

    if (underrun_likely)
        *underrun_likely = (total_frames < BACKEND_REAL_TARGET_FRAMES);
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

static unsigned int ps2_audio_make_room_for_frames(unsigned int wanted_frames)
{
    unsigned int dropped = 0;

    if (wanted_frames >= SOUND_TOTAL_FRAMES)
        wanted_frames = SOUND_TOTAL_FRAMES - 1;

    ps2_audio_ring_lock();

    if (wanted_frames > 0) {
        unsigned int free_frames = SOUND_TOTAL_FRAMES - g_buffered_frames;

        if (free_frames < wanted_frames) {
            dropped = wanted_frames - free_frames;
            if (dropped > g_buffered_frames)
                dropped = g_buffered_frames;

            g_read_pos = (g_read_pos + dropped) % SOUND_TOTAL_FRAMES;
            g_buffered_frames -= dropped;
        }
    }

    ps2_audio_ring_unlock();
    return dropped;
}



static int ps2_audio_backend_queue_all(const int16_t *data, unsigned int frames)
{
    const uint8_t *ptr = (const uint8_t *)data;
    int remaining = (int)(frames * PS2_AUDIO_FRAME_BYTES);
    int total_sent = 0;

    while (remaining > 0) {
        int chunk = remaining;
        int sent;

        if (chunk > BACKEND_FEED_BYTES)
            chunk = BACKEND_FEED_BYTES;

        ps2_backend_wait_audio(chunk);
        SyncDCache((void *)ptr, (void *)(ptr + chunk));

        sent = ps2_backend_queue_audio((const int16_t *)ptr, chunk);
        if (sent <= 0)
            return (total_sent > 0) ? total_sent : -1;
        if (sent > chunk)
            sent = chunk;

        ptr += sent;
        remaining -= sent;
        total_sent += sent;
    }

    return total_sent;
}


static void ps2_audio_fill_backend_silence_to_target(void)
{
    int queued_bytes;

    while (!g_sound_thread_exit) {
        queued_bytes = ps2_backend_queued_bytes();
        if (queued_bytes >= BACKEND_QUEUE_TARGET_BYTES)
            break;

        if (ps2_audio_backend_queue_all(g_menu_silence_block, BACKEND_FEED_FRAMES) < 0)
            break;
    }
}


static int ps2_audio_reprime_backend_with_silence(void)
{
    int ret;

    ret = ps2_backend_reprime_audio(PS2_AUDIO_RATE,
                                    PS2_AUDIO_BYTES_PER_SAMPLE * 8,
                                    PS2_AUDIO_CHANNELS);
    if (ret < 0) {
        PS2AUDIO_LOG("[PS2AUDIO] backend reprime failed -> %d\n", ret);
        return ret;
    }

    memset(g_menu_silence_block, 0, sizeof(g_menu_silence_block));
    ps2_audio_fill_backend_silence_to_target();
    return 0;
}


static void ps2_audio_recover_backend_underrun(void)
{
    if (g_audio_state != 1 || g_audio_paused)
        return;

    g_audio_resume_armed = 1;
    g_warned_underrun = 0;
    ps2_audio_fade_backend_volume(g_audio_backend_volume, 0);

    if (ps2_audio_reprime_backend_with_silence() >= 0) {
        PS2AUDIO_LOG("[PS2AUDIO] backend reprime recovery after underrun\n");
    }
}


static void ps2_audio_sound_thread(void *arg)
{
    int queued_bytes;
    unsigned int buffered_frames;
    unsigned int request_frames;
    unsigned int got_frames;
    int sent_bytes;

    (void)arg;

    g_sound_thread_running = 1;
    PS2AUDIO_LOG("[PS2AUDIO] sound thread start\\n");

    while (!g_sound_thread_exit) {
        if (g_audio_state != 1) {
            ps2_audio_wait_loops(1);
            continue;
        }

        queued_bytes = ps2_backend_queued_bytes();
        buffered_frames = ps2_audio_ring_buffered_frames();

        if (!g_audio_paused &&
            buffered_frames == 0 &&
            queued_bytes <= BACKEND_FEED_BYTES) {
            if (g_backend_empty_streak < AUDIO_UNDERRUN_HYSTERESIS_LOOPS)
                g_backend_empty_streak++;
        } else {
            g_backend_empty_streak = 0;
        }

        if (g_backend_reprime_cooldown > 0)
            g_backend_reprime_cooldown--;

        if (!g_audio_paused &&
            g_backend_reprime_cooldown == 0 &&
            g_backend_empty_streak >= AUDIO_UNDERRUN_HYSTERESIS_LOOPS) {
            ps2_audio_recover_backend_underrun();
            g_backend_reprime_cooldown = 16;
            g_backend_empty_streak = 0;
            ps2_audio_wait_loops(1);
            continue;
        }

        if (g_audio_resume_armed) {
            if (buffered_frames < BACKEND_RESUME_UNMUTE_FRAMES) {
                ps2_audio_wait_loops(1);
                continue;
            }

            ps2_audio_finish_resume_if_ready();
            queued_bytes = ps2_backend_queued_bytes();
        }

        if (queued_bytes >= BACKEND_QUEUE_TARGET_BYTES) {
            ps2_audio_wait_loops(1);
            continue;
        }

        if (g_audio_paused || buffered_frames == 0) {
            if (!g_audio_paused && !g_warned_underrun) {
                PS2AUDIO_LOG("[PS2AUDIO] underrun: topping up silence\\n");
                g_warned_underrun = 1;
            }

            ps2_audio_fill_backend_silence_to_target();
            ps2_audio_wait_loops(1);
            continue;
        }

        if (buffered_frames < BACKEND_FEED_FRAMES &&
            queued_bytes >= BACKEND_REAL_TARGET_BYTES) {
            ps2_audio_wait_loops(1);
            continue;
        }

        request_frames = buffered_frames;
        if (request_frames > BACKEND_FEED_FRAMES)
            request_frames = BACKEND_FEED_FRAMES;

        got_frames = ps2_audio_copy_from_ring(g_backend_block, request_frames);
        if (got_frames == 0) {
            ps2_audio_wait_loops(1);
            continue;
        }

        if (got_frames < BACKEND_FEED_FRAMES) {
            memset(&g_backend_block[got_frames * PS2_AUDIO_CHANNELS],
                   0,
                   (BACKEND_FEED_FRAMES - got_frames) * PS2_AUDIO_FRAME_BYTES);

            if (!g_warned_underrun) {
                PS2AUDIO_LOG("[PS2AUDIO] short block padded: real=%u target=%u\\n",
                             got_frames,
                             (unsigned int)BACKEND_FEED_FRAMES);
                g_warned_underrun = 1;
            }
        } else {
            g_warned_underrun = 0;
        }

        sent_bytes = ps2_audio_backend_queue_all(g_backend_block, BACKEND_FEED_FRAMES);
        if (sent_bytes < 0) {
            PS2AUDIO_LOG("[PS2AUDIO] backend_queue_audio FAIL\\n");
            ps2_audio_wait_loops(4);
            continue;
        }

        if (sent_bytes != BACKEND_FEED_BYTES) {
            PS2AUDIO_LOG("[PS2AUDIO] backend accepted partial=%d expected=%u\\n",
                         sent_bytes,
                         (unsigned int)BACKEND_FEED_BYTES);
        }
    }

    g_sound_thread_running = 0;
    PS2AUDIO_LOG("[PS2AUDIO] sound thread end\\n");
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

void ps2_audio_set_iop_ready(int ready)
{
    ps2_audio_backend_set_iop_ready(ready);
}

int ps2_audio_init_once(void)
{
    int ret;

    if (g_audio_state != 0) {
        PS2AUDIO_LOG("[PS2AUDIO] init skipped state=%d\n", g_audio_state);
        return g_audio_state > 0;
    }

    PS2AUDIO_LOG("[PS2AUDIO] init enter (phase 1 split)\n");

    if (!ps2_audio_backend_iop_ready()) {
        if (!ps2_audio_backend_reset_iop()) {
            g_audio_state = -1;
            PS2AUDIO_LOG("[PS2AUDIO] FAIL: backend_reset_iop\n");
            return 0;
        }
    }

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
    ps2_audio_set_backend_volume(PS2_AUDIO_BACKEND_VOLUME);

    ps2_audio_log_state("init success");
    return 1;
}


void ps2_audio_pump(void)
{
    if (g_audio_state != 1 || !g_audio_paused)
        return;

    if (g_sound_thread_running)
        return;

    (void)ps2_audio_backend_queue_all(g_menu_silence_block, BACKEND_FEED_FRAMES);
}






void ps2_audio_pause(void)
{
    if (g_audio_state != 1)
        return;

    ps2_audio_fade_backend_volume(g_audio_backend_volume, 0);

    g_audio_paused = 1;
    g_audio_resume_armed = 0;
    g_backend_reprime_cooldown = 0;
    g_backend_empty_streak = 0;

    /* pause suave: fade-out, depois re-prime o backend com silencio */
    ps2_audio_clear_ring();
    ps2_audio_reset_stream_state();
    memset(g_menu_silence_block, 0, sizeof(g_menu_silence_block));
    (void)ps2_audio_reprime_backend_with_silence();

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
    g_backend_reprime_cooldown = 0;
    g_backend_empty_streak = 0;
    ps2_audio_clear_ring();
    ps2_audio_reset_stream_state();
    g_warned_not_ready = 0;
    g_warned_overrun = 0;
    g_warned_underrun = 0;
    g_logged_first_push = 0;

    ps2_audio_set_backend_volume(0);
    (void)ps2_audio_reprime_backend_with_silence();

    ps2_audio_log_state("resumed-armed");
}







void ps2_audio_shutdown(void)
{
    ps2_audio_stop_thread();

    if (g_audio_ring_sema >= 0) {
        DeleteSema(g_audio_ring_sema);
        g_audio_ring_sema = -1;
    }

    memset(g_backend_block, 0, sizeof(g_backend_block));
    memset(g_menu_silence_block, 0, sizeof(g_menu_silence_block));
    memset(g_ringbuf, 0, sizeof(g_ringbuf));
    g_write_pos = 0;
    g_read_pos = 0;
    g_buffered_frames = 0;

    g_warned_not_ready = 0;
    g_warned_overrun = 0;
    g_warned_underrun = 0;
    g_logged_first_push = 0;

    g_sound_thread_running = 0;
    g_sound_thread_exit = 0;
    g_audio_paused = 0;
    g_audio_resume_armed = 0;
    g_audio_backend_volume = PS2_AUDIO_BACKEND_VOLUME;
    g_backend_reprime_cooldown = 0;
    g_backend_empty_streak = 0;
    g_audio_state = 0;

    ps2_backend_shutdown();
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

    if (!data || frames == 0)
        return 0;

    if (!g_logged_first_push) {
        PS2AUDIO_LOG("[PS2AUDIO] first push frames=%u\n", (unsigned)frames);
        g_logged_first_push = 1;
    }

    while (done < frames) {
        size_t chunk = frames - done;
        unsigned int written;

        if (chunk >= SOUND_TOTAL_FRAMES) {
            if (!g_warned_overrun) {
                PS2AUDIO_LOG("[PS2AUDIO] oversized audio chunk=%u, trimming to ring\n",
                       (unsigned int)chunk);
                g_warned_overrun = 1;
            }
            chunk = SOUND_TOTAL_FRAMES - 1;
        }

        if (ps2_audio_ring_buffered_frames() + chunk >= SOUND_TOTAL_FRAMES) {
            unsigned int dropped = ps2_audio_make_room_for_frames((unsigned int)chunk);

            if (!g_warned_overrun) {
                PS2AUDIO_LOG("[PS2AUDIO] ring overrun: dropped_oldest=%u incoming=%u\n",
                       dropped, (unsigned int)chunk);
                g_warned_overrun = 1;
            }
        }

        written = ps2_audio_copy_to_ring(&data[done * PS2_AUDIO_CHANNELS], (unsigned int)chunk);
        if (written < chunk) {
            if (!g_warned_overrun) {
                PS2AUDIO_LOG("[PS2AUDIO] ring short write: written=%u expected=%u\n",
                       written, (unsigned int)chunk);
                g_warned_overrun = 1;
            }
        } else {
            g_warned_overrun = 0;
            ps2_audio_finish_resume_if_ready();
        }

        done += chunk;
    }

    return done;
}
