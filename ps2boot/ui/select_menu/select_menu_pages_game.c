#include "select_menu_pages_internal.h"
#include <stdio.h>

static const char *frame_limit_label(int mode)
{
    if (mode == SELECT_MENU_FRAME_LIMIT_50)
        return "50";
    if (mode == SELECT_MENU_FRAME_LIMIT_60)
        return "60";
    if (mode == SELECT_MENU_FRAME_LIMIT_OFF)
        return "OFF";
    return "AUTO";
}

static const char *onoff_label(int v)
{
    return v ? "ON" : "OFF";
}

static const char *slowdown_label(int mode)
{
    if (mode == 1)
        return "COMPATIBLE";
    if (mode == 2)
        return "MAX";
    return "DISABLED";
}

static void draw_option_line(unsigned y, const char *label, int selected)
{
    unsigned text_x = select_menu_pages_center_x_for_text(label);

    if (selected) {
        unsigned arrow_x = (text_x >= 12) ? (text_x - 12) : 0;
        select_menu_font_draw_string_color(arrow_x, y, ">", SELECT_MENU_COLOR_HIGHLIGHT);
    }

    select_menu_font_draw_string_color(
        text_x, y, label,
        selected ? SELECT_MENU_COLOR_HIGHLIGHT : SELECT_MENU_COLOR_TEXT);
}

void select_menu_pages_draw_game_page(const select_menu_state_t *state)
{
    const uint16_t gray = 0x9CF3;
    char buf[64];
    int vsync_index = state->show_fps ? 2 : 1;
    int frame_index = state->show_fps ? 3 : 2;
    int slowdown_index = state->show_fps ? 4 : 3;
    int flicker_index = state->show_fps ? 5 : 4;
    unsigned y = 76;

    select_menu_font_draw_string_color(
        select_menu_pages_center_x_for_text("GAME OPTIONS"),
        SELECT_MENU_TITLE_Y,
        "GAME OPTIONS",
        SELECT_MENU_COLOR_TEXT);

    select_menu_pages_draw_header_line(34, gray);

    snprintf(buf, sizeof(buf), "SHOW FPS: %s", onoff_label(state->show_fps));
    draw_option_line(y, buf, state->game_sel == 0);
    y += 16;

    if (state->show_fps) {
        snprintf(buf, sizeof(buf), "RAINBOW FPS: %s", onoff_label(state->fps_rainbow));
        draw_option_line(y, buf, state->game_sel == 1);
        y += 16;
    }

    snprintf(buf, sizeof(buf), "GAME VSYNC: %s", onoff_label(state->game_vsync));
    draw_option_line(y, buf, state->game_sel == vsync_index);
    y += 16;

    snprintf(buf, sizeof(buf), "FRAME LIMIT: %s", frame_limit_label(state->frame_limit));
    draw_option_line(y, buf, state->game_sel == frame_index);
    y += 16;

    snprintf(buf, sizeof(buf), "REDUCE SLOWDOWN: %s", slowdown_label(state->game_reduce_slowdown));
    draw_option_line(y, buf, state->game_sel == slowdown_index);
    y += 16;

    snprintf(buf, sizeof(buf), "REDUCE FLICKER: %s", onoff_label(state->game_reduce_flicker));
    draw_option_line(y, buf, state->game_sel == flicker_index);

    select_menu_pages_draw_footer_line(172, gray);
    select_menu_pages_draw_footer_actions("CHANGE", "BACK", "CLOSE");
}
