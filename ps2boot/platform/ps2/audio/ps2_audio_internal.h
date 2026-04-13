#ifndef PS2_AUDIO_INTERNAL_H
#define PS2_AUDIO_INTERNAL_H

#include "ps2_audio.h"

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "ps2_audio_backend.h"

#define QUIET_PS2AUDIO_LOGS 1
#if QUIET_PS2AUDIO_LOGS
#define PS2AUDIO_LOG(...) ((void)0)
#else
#define PS2AUDIO_LOG(...) printf(__VA_ARGS__)
#endif

#define PS2_AUDIO_RATE              44100
#define CORE_AUDIO_RATE             32040
#define RESAMPLE_OUT_MAX_FRAMES     1024
#define PS2_AUDIO_CHANNELS          2
#define PS2_AUDIO_BYTES_PER_SAMPLE  2
#define PS2_AUDIO_FRAME_BYTES       (PS2_AUDIO_CHANNELS * PS2_AUDIO_BYTES_PER_SAMPLE)
#define PS2_AUDIO_BACKEND_VOLUME    0x3fff
#define PS2_AUDIO_FADE_STEPS       6
#define PS2_AUDIO_FADE_DELAY_US    750

#define SOUND_BLOCK_COUNT 8
#define SOUND_BUFFER_CHUNK          1024
#define SOUND_TOTAL_FRAMES          (SOUND_BLOCK_COUNT * SOUND_BUFFER_CHUNK)
#define SOUND_TOTAL_BYTES           (SOUND_TOTAL_FRAMES * PS2_AUDIO_FRAME_BYTES)

#define BACKEND_FEED_FRAMES         512
#define BACKEND_FEED_BYTES          (BACKEND_FEED_FRAMES * PS2_AUDIO_FRAME_BYTES)
#define BACKEND_QUEUE_TARGET_FRAMES (BACKEND_FEED_FRAMES * 4)
#define BACKEND_QUEUE_TARGET_BYTES  (BACKEND_QUEUE_TARGET_FRAMES * PS2_AUDIO_FRAME_BYTES)
#define BACKEND_RESUME_UNMUTE_FRAMES (BACKEND_FEED_FRAMES * 4)
#define BACKEND_REAL_TARGET_FRAMES  (BACKEND_FEED_FRAMES * 2)
#define BACKEND_REAL_TARGET_BYTES   (BACKEND_REAL_TARGET_FRAMES * PS2_AUDIO_FRAME_BYTES)
#define AUDIO_SYNC_TARGET_FRAMES     (BACKEND_QUEUE_TARGET_FRAMES + BACKEND_FEED_FRAMES)
#define AUDIO_SYNC_DEADZONE_FRAMES   (BACKEND_FEED_FRAMES / 2)
#define RESAMPLE_BASE_STEP_Q16      (((unsigned int)CORE_AUDIO_RATE) << 16)
#define RESAMPLE_STEP_MAX_DELTA_Q16 (RESAMPLE_BASE_STEP_Q16 / 100)
#define RESAMPLE_STEP_SLEW_MAX_Q16  (RESAMPLE_BASE_STEP_Q16 / 4096)
#define RESAMPLE_IN_CHUNK_MAX       736

#define SOUND_THREAD_PRIO           96
#define SOUND_THREAD_STACK          0x2000


extern int16_t g_resample_out[RESAMPLE_OUT_MAX_FRAMES * PS2_AUDIO_CHANNELS];
extern unsigned int g_resample_phase;
extern unsigned int g_resample_step_q16;
extern unsigned int g_resample_target_step_q16;
extern int16_t g_resample_prev_l;
extern int16_t g_resample_prev_r;
extern int g_resample_have_prev;


size_t ps2_audio_resample_chunk(const int16_t *data, size_t in_frames);

#endif
