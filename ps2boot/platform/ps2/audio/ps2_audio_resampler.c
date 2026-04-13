#include "ps2_audio_internal.h"

int16_t g_resample_out[RESAMPLE_OUT_MAX_FRAMES * PS2_AUDIO_CHANNELS] __attribute__((aligned(64)));
unsigned int g_resample_phase = 0;
unsigned int g_resample_step_q16 = RESAMPLE_BASE_STEP_Q16;
int16_t g_resample_prev_l = 0;
int16_t g_resample_prev_r = 0;
int g_resample_have_prev = 0;

size_t ps2_audio_resample_chunk(const int16_t *data, size_t in_frames)
{
    size_t out_frames = 0;
    size_t i;
    const unsigned int out_rate_q16 = ((unsigned int)PS2_AUDIO_RATE) << 16;
    unsigned int step_q16 = g_resample_step_q16;

    if (!data || in_frames == 0)
        return 0;

    if (step_q16 == 0)
        step_q16 = RESAMPLE_BASE_STEP_Q16;

    if (!g_resample_have_prev) {
        g_resample_prev_l = data[0];
        g_resample_prev_r = data[1];
        g_resample_have_prev = 1;
    }

    for (i = 0; i < in_frames && out_frames < RESAMPLE_OUT_MAX_FRAMES; i++) {
        int16_t cur_l = data[i * 2 + 0];
        int16_t cur_r = data[i * 2 + 1];

        while (g_resample_phase < out_rate_q16 && out_frames < RESAMPLE_OUT_MAX_FRAMES) {
            unsigned int a = g_resample_phase >> 16;
            unsigned int b = (unsigned int)PS2_AUDIO_RATE - a;

            g_resample_out[out_frames * 2 + 0] =
                (int16_t)(((int)g_resample_prev_l * (int)b + (int)cur_l * (int)a) / (int)PS2_AUDIO_RATE);
            g_resample_out[out_frames * 2 + 1] =
                (int16_t)(((int)g_resample_prev_r * (int)b + (int)cur_r * (int)a) / (int)PS2_AUDIO_RATE);

            out_frames++;
            g_resample_phase += step_q16;
        }

        while (g_resample_phase >= out_rate_q16)
            g_resample_phase -= out_rate_q16;

        g_resample_prev_l = cur_l;
        g_resample_prev_r = cur_r;
    }

    return out_frames;
}
