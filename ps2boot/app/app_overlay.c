#include "app_overlay.h"

#include <stdio.h>
#include <time.h>

#include "ps2_menu.h"
#include "ps2_video.h"

static unsigned g_fps_display = 0;
static unsigned g_fps_accum = 0;
static clock_t g_fps_last_clock = 0;
static double g_core_nominal_fps = 60.0;

void app_overlay_reset_timing(void)
{
    g_fps_display = 0;
    g_fps_accum = 0;
    g_fps_last_clock = 0;
    g_core_nominal_fps = 60.0;
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
    /* desativado para diagnosticar travamento no NetherSX2 */
}

void app_overlay_update_fps(void)
{
    clock_t now = clock();
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
        snprintf(l1, sizeof(l1), "FPS: %u", g_fps_display);
        snprintf(l2, sizeof(l2), "LIMIT: OFF");
        ps2_video_set_debug(l1, l2, "", "");
    } else {
        ps2_video_set_debug("", "", "", "");
    }
}
