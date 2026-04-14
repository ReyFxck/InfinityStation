#include "app_overlay.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <graph.h>

#include "ps2_menu.h"
#include "ps2_video.h"
#include "ui/select_menu/select_menu_state.h"

extern const char *app_core_prof_get_line1(void);
extern const char *app_core_prof_get_line2(void);
extern const char *ps2_video_prof_get_line1(void);
extern const char *ps2_video_prof_get_line2(void);

static unsigned g_fps_display = 0;
static unsigned g_fps_accum = 0;
static clock_t g_fps_last_clock = 0;
static double g_core_nominal_fps = 60.0;
static unsigned g_throttle_vblank_accum = 0;
static unsigned g_throttle_last_target_hz = 0;
static unsigned g_throttle_last_display_hz = 0;
static int g_overlay_visible = -1;
static unsigned g_overlay_last_sent_fps = (unsigned)-1;


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

static unsigned app_overlay_display_hz(void)
{
    int region = graph_get_region();
    return (region == GRAPH_MODE_PAL) ? 50u : 60u;
}

static unsigned app_overlay_target_hz(double fps)
{
    unsigned hz;

    if (fps <= 1.0)
        return 0;

    hz = (unsigned)(fps + 0.5);
    if (hz < 1)
        hz = 1;

    return hz;
}

static void app_overlay_reset_vblank_cadence(void)
{
    g_throttle_vblank_accum = 0;
    g_throttle_last_target_hz = 0;
    g_throttle_last_display_hz = 0;
}

void app_overlay_reset_timing(void)
{
    g_fps_display = 0;
    g_fps_accum = 0;
    g_fps_last_clock = 0;
    g_core_nominal_fps = 60.0;
    app_overlay_reset_vblank_cadence();
    g_overlay_visible = -1;
    g_overlay_last_sent_fps = (unsigned)-1;

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

unsigned app_overlay_video_wait_vblanks(int game_vsync_enabled)
{
    double target_fps;
    unsigned display_hz;
    unsigned target_hz;
    unsigned waits;

    if (game_vsync_enabled) {
        app_overlay_reset_vblank_cadence();
        return 1;
    }

    target_fps = app_overlay_target_fps();
    if (target_fps <= 1.0) {
        app_overlay_reset_vblank_cadence();
        return 0;
    }

    display_hz = app_overlay_display_hz();
    target_hz = app_overlay_target_hz(target_fps);

    if (target_hz > display_hz)
        target_hz = display_hz;

    if (g_throttle_last_target_hz != target_hz ||
        g_throttle_last_display_hz != display_hz) {
        app_overlay_reset_vblank_cadence();
        g_throttle_last_target_hz = target_hz;
        g_throttle_last_display_hz = display_hz;
    }

    if (target_hz >= display_hz)
        return 1;

    g_throttle_vblank_accum += display_hz;
    waits = g_throttle_vblank_accum / target_hz;
    g_throttle_vblank_accum %= target_hz;

    if (waits < 1)
        waits = 1;

    return waits;
}

void app_overlay_throttle_if_needed(void)
{
}

void app_overlay_update_fps(void)
{
    clock_t now = clock();
    char l1[32];
    int show_fps;

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

    if (show_fps) {
        if (g_overlay_visible == 1 &&
            g_overlay_last_sent_fps == g_fps_display)
            return;

        snprintf(l1, sizeof(l1), "FPS %u", g_fps_display);
        ps2_video_set_debug(
            l1,
            app_core_prof_get_line1(),
            app_core_prof_get_line2(),
            ps2_video_prof_get_line1()
        );
        g_overlay_visible = 1;
        g_overlay_last_sent_fps = g_fps_display;
    } else {
        if (g_overlay_visible == 0)
            return;

        ps2_video_set_debug("", "", "", "");
        g_overlay_visible = 0;
        g_overlay_last_sent_fps = (unsigned)-1;
    }
}
