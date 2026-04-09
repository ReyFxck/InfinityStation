#ifndef PS2_AUDIO_RING_H
#define PS2_AUDIO_RING_H

#include "ps2_audio_internal.h"

typedef struct ps2_audio_ring_s {
    int sema_id;
    volatile unsigned int write_pos;
    volatile unsigned int read_pos;
    volatile unsigned int buffered_frames;
    int16_t data[SOUND_TOTAL_FRAMES * PS2_AUDIO_CHANNELS];
} ps2_audio_ring_t;

int ps2_audio_ring_create(ps2_audio_ring_t *ring);
void ps2_audio_ring_destroy(ps2_audio_ring_t *ring);
void ps2_audio_ring_clear(ps2_audio_ring_t *ring);
unsigned int ps2_audio_ring_buffered(ps2_audio_ring_t *ring);
unsigned int ps2_audio_ring_free(ps2_audio_ring_t *ring);
unsigned int ps2_audio_ring_read(ps2_audio_ring_t *ring, int16_t *dst, unsigned int frames);
unsigned int ps2_audio_ring_write(ps2_audio_ring_t *ring, const int16_t *src, unsigned int frames);

#endif
