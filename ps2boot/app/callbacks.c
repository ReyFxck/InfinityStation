#include "callbacks.h"
#include "state.h"

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "libretro.h"
#include "video.h"
#include "input.h"
#include "overlay.h"
#include "audio.h"
#include "common/inf_log.h"

/* QUIET_RUNTIME_LOGS used to be defined here; logging is now controlled
 * globally via INF_LOG_LEVEL in common/inf_log.h. */
#define QUIET_RUNTIME_LOGS (INF_LOG_LEVEL == 0)

#define DEBUG_OVERLAY 0
#define APP_MAX_CORE_VARS 32
#define APP_CORE_KEY_MAX 64
#define APP_CORE_VALUE_MAX 64

typedef struct app_core_var {
    char key[APP_CORE_KEY_MAX];
    char value[APP_CORE_VALUE_MAX];
} app_core_var_t;

static unsigned g_frame_count = 0;
static int g_logged_video_cb = 0;
static int g_warned_bad_pixel_format = 0;
static enum retro_pixel_format g_pixel_format = RETRO_PIXEL_FORMAT_RGB565;
static int g_logged_audio_cb = 0;
static int g_logged_audio_batch_cb = 0;
static void (*g_audio_buffer_status_cb)(bool, unsigned, bool) = NULL;

static app_core_var_t g_core_vars[APP_MAX_CORE_VARS];
static unsigned g_core_var_count = 0;
static bool g_core_vars_dirty = false;

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

static void app_retro_log(enum retro_log_level level, const char *fmt, ...)
{
    (void)level;
#if !QUIET_RUNTIME_LOGS
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
#else
    (void)fmt;
#endif
}

static void app_core_vars_clear(void)
{
    memset(g_core_vars, 0, sizeof(g_core_vars));
    g_core_var_count = 0;
}

static void app_copy_core_token(char *dst, size_t dst_size, const char *src)
{
    size_t i = 0;

    if (!dst || dst_size == 0) {
        return;
    }

    if (!src) {
        dst[0] = '\0';
        return;
    }

    while (src[i] && src[i] != '|' && src[i] != ';' && i + 1 < dst_size) {
        dst[i] = src[i];
        i++;
    }

    dst[i] = '\0';
}

static void app_parse_default_value(const char *value_str, char *dst, size_t dst_size)
{
    const char *p;

    if (!dst || dst_size == 0) {
        return;
    }

    dst[0] = '\0';

    if (!value_str) {
        return;
    }

    p = strchr(value_str, ';');
    if (p) {
        p++;
        while (*p == ' ')
            p++;
        app_copy_core_token(dst, dst_size, p);
        return;
    }

    app_copy_core_token(dst, dst_size, value_str);
}

static const char *app_core_override_value(const char *key, const char *parsed_default)
{
    if (!key || !parsed_default) {
        return parsed_default;
    }

    /* Defaults mais amigáveis ao PS2 */
    if (!strcmp(key, "snes9x_2005_frameskip"))
        return "auto";
    if (!strcmp(key, "snes9x_2005_frameskip_threshold"))
        return "33";
    if (!strcmp(key, "snes9x_2005_overclock_cycles"))
        return "compatible";

    return parsed_default;
}

static void app_store_core_variables(const struct retro_variable *vars)
{
    unsigned i = 0;

    app_core_vars_clear();

    if (!vars)
        return;

    while (vars[i].key && g_core_var_count < APP_MAX_CORE_VARS) {
        char parsed_default[APP_CORE_VALUE_MAX];
        const char *final_value;
        app_core_var_t *dst = &g_core_vars[g_core_var_count];

        if (!INF_SNPRINTF_OK(dst->key, sizeof(dst->key), "%s", vars[i].key)) {
            /* Trunca a chave -> ignora a entrada inteira: uma chave parcial
             * vai bater com a entrada errada quando o core consultar. */
            dst->key[0] = '\0';
            i++;
            continue;
        }
        app_parse_default_value(vars[i].value, parsed_default, sizeof(parsed_default));
        final_value = app_core_override_value(dst->key, parsed_default);
        if (!INF_SNPRINTF_OK(dst->value, sizeof(dst->value), "%s",
                             final_value ? final_value : "")) {
            /* Valor truncado -> usa string vazia em vez de um valor partido. */
            dst->value[0] = '\0';
        }

        g_core_var_count++;
        i++;
    }

    g_core_vars_dirty = true;
}

static bool app_get_core_variable(struct retro_variable *var)
{
    unsigned i;

    if (!var || !var->key)
        return false;

    for (i = 0; i < g_core_var_count; i++) {
        if (!strcmp(g_core_vars[i].key, var->key)) {
            var->value = g_core_vars[i].value;
            return true;
        }
    }

    var->value = NULL;
    return false;
}

static bool app_set_pixel_format(const enum retro_pixel_format *fmt)
{
    if (!fmt)
        return false;

    switch (*fmt) {
    case RETRO_PIXEL_FORMAT_RGB565:
        g_pixel_format = RETRO_PIXEL_FORMAT_RGB565;
        g_warned_bad_pixel_format = 0;
        return true;

    default:
#if !QUIET_RUNTIME_LOGS
        if (!g_warned_bad_pixel_format) {
            printf("[APPCB] unsupported pixel format requested: %d\n", (int)*fmt);
            g_warned_bad_pixel_format = 1;
        }
#else
        g_warned_bad_pixel_format = 1;
#endif
        return false;
    }
}

static bool environ_cb(unsigned cmd, void *data)
{
    switch (cmd) {
    case RETRO_ENVIRONMENT_GET_CAN_DUPE:
        if (data) {
            *(bool *)data = true;
            return true;
        }
        return false;

    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        return app_set_pixel_format((const enum retro_pixel_format *)data);

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

    case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:
        if (data) {
            *(unsigned *)data = 0;
            return true;
        }
        return false;

    case RETRO_ENVIRONMENT_GET_LANGUAGE:
        if (data) {
            *(unsigned *)data = RETRO_LANGUAGE_ENGLISH;
            return true;
        }
        return false;

    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
        if (data) {
            *(bool *)data = g_core_vars_dirty;
            g_core_vars_dirty = false;
            return true;
        }
        return false;

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

    case RETRO_ENVIRONMENT_SET_VARIABLES:
        app_store_core_variables((const struct retro_variable *)data);
        return true;

    case RETRO_ENVIRONMENT_GET_VARIABLE:
        return app_get_core_variable((struct retro_variable *)data);

    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        if (data) {
            struct retro_log_callback *cb = (struct retro_log_callback *)data;
            cb->log = app_retro_log;
            return true;
        }
        return false;

    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
    case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
    case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
    case RETRO_ENVIRONMENT_SET_GEOMETRY:
    case RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO:
    case RETRO_ENVIRONMENT_SET_MESSAGE:
        return true;

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

    if (g_pixel_format != RETRO_PIXEL_FORMAT_RGB565) {
#if !QUIET_RUNTIME_LOGS
        if (!g_warned_bad_pixel_format) {
            printf("[APPCB] refusing frame with unsupported pixel format=%d\n",
                   (int)g_pixel_format);
            g_warned_bad_pixel_format = 1;
        }
#endif
        return;
    }

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
    (void)index;

    if (port >= PS2_INPUT_MAX_PLAYERS)
        return 0;

    if (device != RETRO_DEVICE_JOYPAD)
        return 0;

    return ps2_input_libretro_state(port, id);
}

void app_callbacks_register(void)
{
    retro_set_environment(environ_cb);
    retro_set_video_refresh(video_cb);
    printf("[APPCB] registering audio callbacks (batch only)\n");
    /* audio_cb (single-sample) nao eh usado em USE_BLARGG_APU; se fosse
     * acionado adquiria o semaforo do ring por amostra. Deixe NULL. */
    retro_set_audio_sample(NULL);
    retro_set_audio_sample_batch(audio_batch_cb);
    printf("[APPCB] audio callbacks registered\n");
    retro_set_input_poll(input_poll_cb);
    retro_set_input_state(input_state_cb);
}
