#include "launcher_render.h"

#include "launcher_font.h"
#include "font/browser_font.h"
#include "ps2_video.h"
#include "ps2_launcher_video.h"
#include "launcher_pages.h"
#include "launcher_background.h"

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

    browser_font_draw_string_color_sized(162, 76, "SUPER NINTENDO EMULATOR FOR THE PS2", sub_shadow, 7, 14);
    browser_font_draw_string_color_sized(161, 75, "SUPER NINTENDO EMULATOR FOR THE PS2", sub_color, 7, 14);

    launcher_draw_credit_underline(161, 91, 270, sub_shadow);
    launcher_draw_credit_underline(160, 90, 270, sub_color);

    browser_font_draw_string_color_sized(8, 58, "ALPHA 1.0", credit_color, 10, 14);
    browser_font_draw_string_color_sized(8, 74, "19.3.2026", credit_color, 10, 14);

    browser_font_draw_string_color_sized(498, 59, "BY: ReyFxck", credit_color, 10, 14);
    browser_font_draw_string_color_sized(498, 75, "Thomas Reis", credit_color, 10, 14);
}

static int point_in_round_rect(int px, int py, int x, int y, int w, int h, int r)
{
    int cx;
    int cy;
    int dx;
    int dy;

    if (px < x || py < y || px >= x + w || py >= y + h)
        return 0;

    if (px >= x + r && px < x + w - r)
        return 1;
    if (py >= y + r && py < y + h - r)
        return 1;

    cx = (px < x + r) ? (x + r - 1) : (x + w - r);
    cy = (py < y + r) ? (y + r - 1) : (y + h - r);
    dx = px - cx;
    dy = py - cy;

    return (dx * dx + dy * dy) <= (r * r);
}

static void draw_glass_panel(int x, int y, int w, int h)
{
    int px;
    int py;
    int r = 18;

    for (py = y + 4; py < y + h + 4; py++) {
        for (px = x + 4; px < x + w + 4; px++) {
            if (!point_in_round_rect(px - 4, py - 4, x, y, w, h, r))
                continue;
            if (((px + py) & 3) == 0)
                ps2_launcher_video_put_pixel((unsigned)px, (unsigned)py, 0x0000);
        }
    }

    for (py = y; py < y + h; py++) {
        for (px = x; px < x + w; px++) {
            int outer;
            int inner;
            int top_band;

            if (!point_in_round_rect(px, py, x, y, w, h, r))
                continue;

            outer = (px < x + 2 || px >= x + w - 2 || py < y + 2 || py >= y + h - 2);
            inner = (px < x + 4 || px >= x + w - 4 || py < y + 4 || py >= y + h - 4);
            top_band = (py >= y + 3 && py < y + 30);

            if (outer) {
                ps2_launcher_video_put_pixel((unsigned)px, (unsigned)py, 0xFFFF);
                continue;
            }

            if (inner) {
                if (((px ^ py) & 1) == 0)
                    ps2_launcher_video_put_pixel((unsigned)px, (unsigned)py, 0xEF7D);
                continue;
            }

            if (top_band) {
                if (((px + py) & 1) == 0)
                    ps2_launcher_video_put_pixel((unsigned)px, (unsigned)py, 0xFFFF);
                continue;
            }

            if (((px ^ py) & 1) == 0)
                ps2_launcher_video_put_pixel((unsigned)px, (unsigned)py, 0xEF5D);
        }
    }
}

void launcher_render(const launcher_state_t *state,
                     const launcher_browser_state_t *browser_state)
{
    if (!state)
        return;

    if (!ps2_launcher_video_init_once())
        return;

    ps2_launcher_video_begin_frame(0);
    launcher_background_draw();

    if (state->page == LAUNCHER_PAGE_MAIN) {
        draw_glass_panel(180, 125, 280, 220);
    } else if (state->page == LAUNCHER_PAGE_BROWSER ||
               state->page == LAUNCHER_PAGE_CREDITS) {
        draw_glass_panel(95, 95, 450, 285);
    }

    launcher_pages_draw(state, browser_state);
    launcher_draw_credit_overlay();
    ps2_launcher_video_end_frame();
}
