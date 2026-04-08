#include "app_callbacks.h"
#include "app_state.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "libretro.h"
#include "ps2_video.h"
#include "ps2_input.h"
#include "app_overlay.h"
#include "platform/ps2/ps2_audio.h"

/* QUIET_RUNTIME_LOGS_BEGIN */
#define QUIET_RUNTIME_LOGS 1
#if QUIET_RUNTIME_LOGS
#undef printf
#define printf(...) ((void)0)
#endif
/* QUIET_RUNTIME_LOGS_END */

#define DEBUG_OVERLAY 0

static unsigned g_frame_count = 0;
static int g_logged_video_cb = 0;
static enum retro_pixel_format g_pixel_format = RETRO_PIXEL_FORMAT_RGB565;
static int g_logged_audio_cb = 0;
static int g_logged_audio_batch_cb = 0;
static void (*g_audio_buffer_status_cb)(bool, unsigned, bool) = NULL;

static int app_audio_accepts_core_audio(void)
{
    return app_state_mode() == APP_MODE_GAME;
}

static void app_audio_report_buffer_status(void)
{
    bool active = false;
    bool underrun_likely = false;
    unsigned occupancy = 0;

    if (!g_audio_buffer_status_cb)
        return;

    if (!app_audio_accepts_core_audio()) {
        g_audio_buffer_status_cb(false, 0, false);
        return;
    }

    ps2_audio_get_buffer_status(&active, &occupancy, &underrun_likely);
    g_audio_buffer_status_cb(active, occupancy, underrun_likely);
}

static bool environ_cb(unsigned cmd, void *data)
{
    switch (cmd) {
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        g_pixel_format = *(const enum retro_pixel_format *)data;
        return true;

    case RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK:
        if (!data) {
            g_audio_buffer_status_cb = NULL;
            return true;
        }
        g_audio_buffer_status_cb =
            ((const struct retro_audio_buffer_status_callback *)data)->callback;
        return true;

    case RETRO_ENVIRONMENT_SET_MINIMUM_AUDIO_LATENCY:
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
    if (!g_logged_video_cb) {
        printf("[APPCB] first video_cb %ux%u pitch=%u\n", width, height, (unsigned)pitch);
        g_logged_video_cb = 1;
    }

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

    if (!g_logged_audio_cb) {
        printf("[APPCB] first audio_cb L=%d R=%d\n", left, right);
        g_logged_audio_cb = 1;
    }

    if (!app_audio_accepts_core_audio())
        return;

    ps2_audio_push_samples(tmp, 1);
    app_audio_report_buffer_status();
}

static size_t audio_batch_cb(const int16_t *data, size_t frames)
{
    size_t written;

    if (!g_logged_audio_batch_cb) {
        printf("[APPCB] first audio_batch_cb frames=%u\n", (unsigned)frames);
        g_logged_audio_batch_cb = 1;
    }

    if (!app_audio_accepts_core_audio())
        return frames;

    written = ps2_audio_push_samples(data, frames);
    app_audio_report_buffer_status();
    return written;
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

    printf("[APPCB] registering audio callbacks\n");
    retro_set_audio_sample(audio_cb);
    retro_set_audio_sample_batch(audio_batch_cb);
    printf("[APPCB] audio callbacks registered\n");

    retro_set_input_poll(input_poll_cb);
    retro_set_input_state(input_state_cb);
}
