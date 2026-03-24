#include "app_callbacks.h"

#include <stdio.h>
#include <debug.h>
#include <stdbool.h>
#include <stdint.h>

#include "libretro.h"
#include "ps2_video.h"
#include "ps2_input.h"
#include "app_overlay.h"
#include "platform/ps2/ps2_audio.h"

#define DEBUG_OVERLAY 0

static unsigned g_frame_count = 0;
static enum retro_pixel_format g_pixel_format = RETRO_PIXEL_FORMAT_RGB565;

static bool environ_cb(unsigned cmd, void *data)
{
    switch (cmd) {
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        g_pixel_format = *(const enum retro_pixel_format *)data;
        return true;

    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: {
        const char **dir = (const char **)data;
        *dir = "";
        return true;
    }

    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY: {
        const char **dir = (const char **)data;
        *dir = "";
        return true;
    }

    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
    case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
    case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
    case RETRO_ENVIRONMENT_SET_GEOMETRY:
    case RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO:
    case RETRO_ENVIRONMENT_SET_VARIABLES:
        return true;

    case RETRO_ENVIRONMENT_GET_VARIABLE:
    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        return false;

    default:
        return false;
    }
}

static void video_cb(const void *data, unsigned width, unsigned height, size_t pitch)
{
#if DEBUG_OVERLAY
    char l1[32], l2[32], l3[32], l4[32];
#endif

    g_frame_count++;

#if DEBUG_OVERLAY
    if ((g_frame_count % 30) == 0 || g_frame_count == 1) {
        snprintf(l1, sizeof(l1), "PAD=%04X", (unsigned)ps2_input_buttons());
        snprintf(l2, sizeof(l2), "%ux%u", width, height);
        snprintf(l3, sizeof(l3), "P=%u", (unsigned)pitch);
        snprintf(l4, sizeof(l4), "FMT=%d", g_pixel_format);
        ps2_video_set_debug(l1, l2, l3, l4);
    }
#endif

    app_overlay_update_fps();

    if (g_pixel_format == RETRO_PIXEL_FORMAT_RGB565)
        ps2_video_present_rgb565(data, width, height, pitch);
}

static void audio_cb(int16_t left, int16_t right)
{
    int16_t tmp[2] = { left, right };
    ps2_audio_push_samples(tmp, 1);
}

static size_t audio_batch_cb(const int16_t *data, size_t frames)
{
    return ps2_audio_push_samples(data, frames);
}

static void input_poll_cb(void)
{
}

static int16_t input_state_cb(unsigned port, unsigned device, unsigned index, unsigned id)
{
    (void)port;
    (void)device;
    (void)index;
    return ps2_input_libretro_state(id);
}

void app_callbacks_register(void)
{
    retro_set_environment(environ_cb);
    retro_set_video_refresh(video_cb);
    retro_set_audio_sample(audio_cb);
    retro_set_audio_sample_batch(audio_batch_cb);
    retro_set_input_poll(input_poll_cb);
    retro_set_input_state(input_state_cb);
}
