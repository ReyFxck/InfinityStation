#include "ps2_audio_ring.h"

#include <kernel.h>
#include <string.h>

static void ps2_audio_ring_lock(ps2_audio_ring_t *ring)
{
    if (ring && ring->sema_id >= 0)
        WaitSema(ring->sema_id);
}

static void ps2_audio_ring_unlock(ps2_audio_ring_t *ring)
{
    if (ring && ring->sema_id >= 0)
        SignalSema(ring->sema_id);
}

int ps2_audio_ring_create(ps2_audio_ring_t *ring)
{
    ee_sema_t sema;

    if (!ring)
        return 0;

    memset(ring, 0, sizeof(*ring));
    ring->sema_id = -1;

    memset(&sema, 0, sizeof(sema));
    sema.init_count = 1;
    sema.max_count = 1;

    ring->sema_id = CreateSema(&sema);
    if (ring->sema_id < 0)
        return 0;

    return 1;
}

void ps2_audio_ring_destroy(ps2_audio_ring_t *ring)
{
    if (!ring)
        return;

    if (ring->sema_id >= 0) {
        DeleteSema(ring->sema_id);
        ring->sema_id = -1;
    }

    ring->write_pos = 0;
    ring->read_pos = 0;
    ring->buffered_frames = 0;
}

void ps2_audio_ring_clear(ps2_audio_ring_t *ring)
{
    if (!ring)
        return;

    ps2_audio_ring_lock(ring);
    memset(ring->data, 0, sizeof(ring->data));
    ring->write_pos = 0;
    ring->read_pos = 0;
    ring->buffered_frames = 0;
    ps2_audio_ring_unlock(ring);
}

unsigned int ps2_audio_ring_buffered(ps2_audio_ring_t *ring)
{
    unsigned int v = 0;

    if (!ring)
        return 0;

    ps2_audio_ring_lock(ring);
    v = ring->buffered_frames;
    ps2_audio_ring_unlock(ring);
    return v;
}

unsigned int ps2_audio_ring_free(ps2_audio_ring_t *ring)
{
    unsigned int v = 0;

    if (!ring)
        return 0;

    ps2_audio_ring_lock(ring);
    v = SOUND_TOTAL_FRAMES - ring->buffered_frames;
    ps2_audio_ring_unlock(ring);
    return v;
}

unsigned int ps2_audio_ring_read(ps2_audio_ring_t *ring, int16_t *dst, unsigned int frames)
{
    unsigned int copied = 0;

    if (!ring || !dst || frames == 0)
        return 0;

    ps2_audio_ring_lock(ring);

    while (copied < frames && ring->buffered_frames > 0) {
        unsigned int rp = ring->read_pos;
        unsigned int avail_until_wrap = SOUND_TOTAL_FRAMES - rp;
        unsigned int take = frames - copied;

        if (take > ring->buffered_frames)
            take = ring->buffered_frames;
        if (take > avail_until_wrap)
            take = avail_until_wrap;

        memcpy(&dst[copied * PS2_AUDIO_CHANNELS],
               &ring->data[rp * PS2_AUDIO_CHANNELS],
               take * PS2_AUDIO_FRAME_BYTES);

        ring->read_pos = (ring->read_pos + take) % SOUND_TOTAL_FRAMES;
        ring->buffered_frames -= take;
        copied += take;
    }

    ps2_audio_ring_unlock(ring);
    return copied;
}

unsigned int ps2_audio_ring_write(ps2_audio_ring_t *ring, const int16_t *src, unsigned int frames)
{
    unsigned int written = 0;

    if (!ring || !src || frames == 0)
        return 0;

    ps2_audio_ring_lock(ring);

    while (written < frames) {
        unsigned int free_frames = SOUND_TOTAL_FRAMES - ring->buffered_frames;
        unsigned int wp = ring->write_pos;
        unsigned int avail_until_wrap = SOUND_TOTAL_FRAMES - wp;
        unsigned int put = frames - written;

        if (free_frames == 0)
            break;
        if (put > free_frames)
            put = free_frames;
        if (put > avail_until_wrap)
            put = avail_until_wrap;

        memcpy(&ring->data[wp * PS2_AUDIO_CHANNELS],
               &src[written * PS2_AUDIO_CHANNELS],
               put * PS2_AUDIO_FRAME_BYTES);

        ring->write_pos = (ring->write_pos + put) % SOUND_TOTAL_FRAMES;
        ring->buffered_frames += put;
        written += put;
    }

    ps2_audio_ring_unlock(ring);
    return written;
}
