#include "app_overlay.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <delaythread.h>

#include "ps2_menu.h"
#include "ps2_video.h"
#include "ui/select_menu/select_menu_state.h"

static unsigned g_fps_display = 0;
static unsigned g_fps_accum = 0;
static clock_t g_fps_last_clock = 0;
static double g_core_nominal_fps = 60.0;
static clock_t g_throttle_last_clock = 0;
static int g_overlay_visible = -1;
static unsigned g_overlay_last_sent_fps = (unsigned)-1;
static char g_overlay_last_limit[16] = "";

static double app_overlay_target_fps(void)
{
    int mode = ps2_menu_frame_limit_mode();

    switch (mode) {
        case SELECT_MENU_FRAME_LIMIT_AUTO:
            return (g_core_nominal_fps > 1.0) ? g_core_nominal_fps : 60.0;
        case SELECT_MENU_FRAME_LIMIT_50:
            return 50.0;
        case SELECT_MENU_FRAME_LIMIT_60:
            return 60.0;
        case SELECT_MENU_FRAME_LIMIT_OFF:
        default:
            return 0.0;
    }
}

static const char *app_overlay_frame_limit_label(void)
{
    int mode = ps2_menu_frame_limit_mode();

    switch (mode) {
        case SELECT_MENU_FRAME_LIMIT_AUTO: return "AUTO";
        case SELECT_MENU_FRAME_LIMIT_50:   return "50";
        case SELECT_MENU_FRAME_LIMIT_60:   return "60";
        case SELECT_MENU_FRAME_LIMIT_OFF:
        default:                           return "OFF";
    }
}

void app_overlay_reset_timing(void)
{
    g_fps_display = 0;
    g_fps_accum = 0;
    g_fps_last_clock = 0;
    g_core_nominal_fps = 60.0;
    g_throttle_last_clock = 0;
    g_overlay_visible = -1;
    g_overlay_last_sent_fps = (unsigned)-1;
    g_overlay_last_limit[0] = '\0';
}

void app_overlay_set_core_nominal_fps(double fps)
{
    if (fps > 1.0)
        g_core_nominal_fps = fps;
    else
        g_core_nominal_fps = 60.0;
}

double app_overlay_get_core_nominal_fps(void)
{
    return g_core_nominal_fps;
}

void app_overlay_throttle_if_needed(void)
{
    double target_fps = app_overlay_target_fps();
    clock_t now;
    clock_t frame_ticks;
    clock_t spin_ticks;

    if (target_fps <= 1.0) {
        g_throttle_last_clock = 0;
        return;
    }

    frame_ticks = (clock_t)(((double)CLOCKS_PER_SEC / target_fps) + 0.5);
    if (frame_ticks < 1)
        frame_ticks = 1;

    spin_ticks = CLOCKS_PER_SEC / 1000;
    if (spin_ticks < 1)
        spin_ticks = 1;

    now = clock();

    if (g_throttle_last_clock == 0) {
        g_throttle_last_clock = now;
        return;
    }

    while ((now - g_throttle_last_clock) < frame_ticks) {
        clock_t remaining = frame_ticks - (now - g_throttle_last_clock);

        if (remaining > spin_ticks) {
            int sleep_us = (int)(((double)(remaining - spin_ticks) * 1000000.0) /
                                 (double)CLOCKS_PER_SEC);
            if (sleep_us > 0)
                DelayThread(sleep_us);
        }

        now = clock();
    }

    g_throttle_last_clock += frame_ticks;

    if ((now - g_throttle_last_clock) > (frame_ticks * 4))
        g_throttle_last_clock = now;
}

void app_overlay_update_fps(void)
{
    clock_t now = clock();
    char l1[32];
    char l2[32];
    int show_fps;
    const char *limit_label;

    g_fps_accum++;

    if (g_fps_last_clock == 0)
        g_fps_last_clock = now;

    if ((now - g_fps_last_clock) >= CLOCKS_PER_SEC) {
        g_fps_display = (unsigned)(((double)g_fps_accum * (double)CLOCKS_PER_SEC) /
                                   (double)(now - g_fps_last_clock) + 0.5);
        g_fps_accum = 0;
        g_fps_last_clock = now;
    }

    show_fps = ps2_menu_show_fps_enabled();
    limit_label = app_overlay_frame_limit_label();

    if (show_fps) {
        if (g_overlay_visible == 1 &&
            g_overlay_last_sent_fps == g_fps_display &&
            strcmp(g_overlay_last_limit, limit_label) == 0)
            return;

        snprintf(l1, sizeof(l1), "FPS: %u", g_fps_display);
        snprintf(l2, sizeof(l2), "LIMIT: %s", limit_label);
        ps2_video_set_debug(l1, l2, "", "");

        g_overlay_visible = 1;
        g_overlay_last_sent_fps = g_fps_display;
        snprintf(g_overlay_last_limit, sizeof(g_overlay_last_limit), "%s", limit_label);
    } else {
        if (g_overlay_visible == 0)
            return;

        ps2_video_set_debug("", "", "", "");
        g_overlay_visible = 0;
        g_overlay_last_sent_fps = (unsigned)-1;
        g_overlay_last_limit[0] = '\0';
    }
}
