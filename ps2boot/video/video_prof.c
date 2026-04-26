#include "video_internal.h"
#include <stdio.h>

static unsigned g_prof_frames;
static unsigned long long g_prof_convert_cycles;
static unsigned long long g_prof_overlay_cycles;
static unsigned long long g_prof_backend_cycles;
static unsigned long long g_prof_total_cycles;
static char g_prof_line1[48];
static char g_prof_line2[48];

static inline float ps2_video_prof_cycles_to_ms(unsigned long long cycles, unsigned frames)
{
    if (!frames)
        return 0.0f;

    return (float)((double)cycles / (double)frames / 147456.0);
}

void ps2_video_prof_commit(
    unsigned cvt_cycles,
    unsigned ovl_cycles,
    unsigned backend_cycles,
    unsigned total_cycles,
    unsigned width,
    unsigned height
)
{
    (void)width;
    (void)height;

    g_prof_convert_cycles += cvt_cycles;
    g_prof_overlay_cycles += ovl_cycles;
    g_prof_backend_cycles += backend_cycles;
    g_prof_total_cycles += total_cycles;
    g_prof_frames++;

    if (g_prof_frames >= 30u) {
        snprintf(
            g_prof_line1,
            sizeof(g_prof_line1),
            "VID C%.2f O%.2f G%.2f",
            ps2_video_prof_cycles_to_ms(g_prof_convert_cycles, g_prof_frames),
            ps2_video_prof_cycles_to_ms(g_prof_overlay_cycles, g_prof_frames),
            ps2_video_prof_cycles_to_ms(g_prof_backend_cycles, g_prof_frames)
        );

        snprintf(
            g_prof_line2,
            sizeof(g_prof_line2),
            "VID T%.2f",
            ps2_video_prof_cycles_to_ms(g_prof_total_cycles, g_prof_frames)
        );

        g_prof_frames = 0;
        g_prof_convert_cycles = 0;
        g_prof_overlay_cycles = 0;
        g_prof_backend_cycles = 0;
        g_prof_total_cycles = 0;
    }
}

const char *ps2_video_prof_get_line1(void)
{
    return g_prof_line1;
}

const char *ps2_video_prof_get_line2(void)
{
    return g_prof_line2;
}
