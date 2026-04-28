#include <stdint.h>

#include "render.h"

#include "../font/font.h"
#include "../font/browser_font.h"

#include "video.h"
#include "video_internal.h"
#include "launcher_video.h"

#include "../pages/pages.h"
#include "../background/background.h"

enum {
    LAUNCHER_STATIC_KIND_NONE = -1,
    LAUNCHER_STATIC_KIND_MAIN = 0,
    LAUNCHER_STATIC_KIND_BROWSER = 1,
    LAUNCHER_STATIC_KIND_OPTIONS = 2
};

static void launcher_draw_credit_underline(int x, int y, int w, uint16_t color)
{
    int i;

    for (i = 0; i < w; i++) {
        ps2_video_ui_put_pixel((unsigned)(x + i), (unsigned)y, color);
        ps2_video_ui_put_pixel((unsigned)(x + i), (unsigned)(y + 1), color);
    }
}

static void launcher_draw_credit_overlay(void)
{
    const uint16_t credit_color = 0xC018;
    const uint16_t sub_color = 0xE800;
    const uint16_t sub_shadow = 0x0000;

    browser_font_draw_string_color_sized(162, 76,
                                         "SUPER NINTENDO EMULATOR FOR THE PS2",
                                         sub_shadow, 7, 14);
    browser_font_draw_string_color_sized(161, 75,
                                         "SUPER NINTENDO EMULATOR FOR THE PS2",
                                         sub_color, 7, 14);
    launcher_draw_credit_underline(161, 91, 270, sub_shadow);
    launcher_draw_credit_underline(160, 90, 270, sub_color);

    browser_font_draw_string_color_sized(8, 58, "ALPHA 1.0",
                                         credit_color, 10, 14);
    browser_font_draw_string_color_sized(8, 74, "19.3.2026",
                                         credit_color, 10, 14);
    browser_font_draw_string_color_sized(498, 59, "BY: ReyFxck",
                                         credit_color, 10, 14);
    browser_font_draw_string_color_sized(498, 75, "Thomas Reis",
                                         credit_color, 10, 14);
}

static int launcher_static_kind_for_page(int page)
{
    if (page == LAUNCHER_PAGE_MAIN)
        return LAUNCHER_STATIC_KIND_MAIN;

    if (page == LAUNCHER_PAGE_BROWSER || page == LAUNCHER_PAGE_CREDITS)
        return LAUNCHER_STATIC_KIND_BROWSER;

    return LAUNCHER_STATIC_KIND_OPTIONS;
}

static void launcher_render_static_base(int kind)
{
    /* Builds the per-frame UI layer. The starfield AND the glass
     * panel are now GS-side: the starfield is emitted as sprite
     * primitives and the panel as 2-3 filled quads, both inside
     * launcher_video_end_frame()'s draw packet. All this function
     * does is advance the star simulation, register the panel rect
     * for this frame, and rasterize the (still software-side)
     * credits into the upload texture. */
    ps2_launcher_video_begin_frame(0);

    launcher_background_advance();

    if (kind == LAUNCHER_STATIC_KIND_MAIN)
        ps2_launcher_video_set_panel(180, 125, 280, 220);
    else if (kind == LAUNCHER_STATIC_KIND_BROWSER)
        ps2_launcher_video_set_panel(95, 95, 450, 285);

    launcher_draw_credit_overlay();
}

static void launcher_begin_cached_frame(int kind)
{
    launcher_render_static_base(kind);

    g_ui_target_launcher = 1;
}

void launcher_render(const launcher_state_t *state)
{
    int kind;

    if (!state)
        return;

    if (!ps2_launcher_video_init_once())
        return;

    kind = launcher_static_kind_for_page(state->page);

    launcher_begin_cached_frame(kind);
    launcher_pages_draw(state);
    ps2_launcher_video_end_frame();
}
