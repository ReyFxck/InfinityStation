#include "ps2_audio_internal.h"

int16_t g_resample_out[RESAMPLE_OUT_MAX_FRAMES * PS2_AUDIO_CHANNELS] __attribute__((aligned(64)));
unsigned int g_resample_phase = 0;
int16_t g_resample_prev_l = 0;
int16_t g_resample_prev_r = 0;
int g_resample_have_prev = 0;

size_t ps2_audio_resample_chunk(const int16_t *data, size_t in_frames)
{
    size_t out_frames = 0;
    size_t i;

    if (!data || in_frames == 0)
        return 0;

    if (!g_resample_have_prev) {
        g_resample_prev_l = data[0];
        g_resample_prev_r = data[1];
        g_resample_have_prev = 1;
    }

    for (i = 0; i < in_frames && out_frames < RESAMPLE_OUT_MAX_FRAMES; i++) {
        int16_t cur_l = data[i * 2 + 0];
        int16_t cur_r = data[i * 2 + 1];

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

    return out_frames;
}
