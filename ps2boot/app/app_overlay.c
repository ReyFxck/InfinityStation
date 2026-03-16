#include "app_overlay.h"

#include <stdio.h>
#include <time.h>

#include "ps2_menu.h"
#include "ps2_video.h"

static unsigned g_fps_display = 0;
static unsigned g_fps_accum = 0;
static clock_t g_fps_last_clock = 0;
static double g_core_nominal_fps = 60.0;
static clock_t g_frame_deadline = 0;

static double target_limit_fps(void)
{
    int mode = ps2_menu_frame_limit_mode();

    if (mode == PS2_FRAME_LIMIT_50)
        return 50.0;
    if (mode == PS2_FRAME_LIMIT_60)
        return 60.0;
    if (mode == PS2_FRAME_LIMIT_OFF)
        return 0.0;

    if (g_core_nominal_fps > 1.0)
        return g_core_nominal_fps;

    return 60.0;
}

void app_overlay_reset_timing(void)
{
    g_fps_display = 0;
    g_fps_accum = 0;
    g_fps_last_clock = 0;
    g_core_nominal_fps = 60.0;
    g_frame_deadline = 0;
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
    double fps = target_limit_fps();
    double ticks_per_frame_d;
    clock_t ticks_per_frame;
    clock_t now;

    if (fps <= 0.0)
        return;

    ticks_per_frame_d = (double)CLOCKS_PER_SEC / fps;
    if (ticks_per_frame_d < 1.0)
        ticks_per_frame_d = 1.0;

    ticks_per_frame = (clock_t)(ticks_per_frame_d + 0.5);
    now = clock();

    if (g_frame_deadline == 0) {
        g_frame_deadline = now + ticks_per_frame;
        return;
    }

    if (now < g_frame_deadline) {
        while ((now = clock()) < g_frame_deadline) {}
    } else if ((now - g_frame_deadline) > (ticks_per_frame * 4)) {
        g_frame_deadline = now;
    }

    g_frame_deadline += ticks_per_frame;
}

void app_overlay_update_fps(void)
{
    clock_t now = clock();
    double target = target_limit_fps();
    char l1[32];
    char l2[32];

    g_fps_accum++;

    if (g_fps_last_clock == 0)
        g_fps_last_clock = now;

    if ((now - g_fps_last_clock) >= CLOCKS_PER_SEC) {
        g_fps_display = (unsigned)(((double)g_fps_accum * (double)CLOCKS_PER_SEC) /
                                   (double)(now - g_fps_last_clock) + 0.5);
        g_fps_accum = 0;
        g_fps_last_clock = now;
    }

    if (ps2_menu_show_fps_enabled()) {
        if (target > 0.0)
            snprintf(l1, sizeof(l1), "FPS: %u/%.0f", g_fps_display, target);
        else
            snprintf(l1, sizeof(l1), "FPS: %u/OFF", g_fps_display);

        if (ps2_menu_frame_limit_mode() == PS2_FRAME_LIMIT_AUTO)
            snprintf(l2, sizeof(l2), "LIMIT: AUTO");
        else if (ps2_menu_frame_limit_mode() == PS2_FRAME_LIMIT_50)
            snprintf(l2, sizeof(l2), "LIMIT: 50");
        else if (ps2_menu_frame_limit_mode() == PS2_FRAME_LIMIT_60)
            snprintf(l2, sizeof(l2), "LIMIT: 60");
        else
            snprintf(l2, sizeof(l2), "LIMIT: OFF");

        if (ps2_menu_fps_rainbow_enabled()) {
            char l2fx[64];
            snprintf(l2fx, sizeof(l2fx), "%.56s MORE FPS", l2);
            ps2_video_set_debug(l1, l2fx, "", "");
        } else {
            ps2_video_set_debug(l1, l2, "", "");
        }
    } else {
        ps2_video_set_debug("", "", "", "");
    }
}
