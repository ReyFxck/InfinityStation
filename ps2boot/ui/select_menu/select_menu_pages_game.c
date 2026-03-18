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

static void draw_option_line(unsigned y, const char *label, int selected)
{
    unsigned text_x = select_menu_pages_center_x_for_text(label);

    if (selected) {
        unsigned arrow_x = (text_x >= 12) ? (text_x - 12) : 0;
        select_menu_font_draw_string_color(arrow_x, y, ">", SELECT_MENU_COLOR_HIGHLIGHT);
    }

    select_menu_font_draw_string_color(
        text_x,
        y,
        label,
        selected ? SELECT_MENU_COLOR_HIGHLIGHT : SELECT_MENU_COLOR_TEXT
    );
}

void select_menu_pages_draw_game_page(const select_menu_state_t *state)
{
    const uint16_t gray = 0x9CF3;
    char buf[64];

    select_menu_font_draw_string_color(
        select_menu_pages_center_x_for_text("GAME OPTIONS"),
        SELECT_MENU_TITLE_Y,
        "GAME OPTIONS",
        SELECT_MENU_COLOR_TEXT
    );

    select_menu_pages_draw_header_line(34, gray);

    snprintf(buf, sizeof(buf), "SHOW FPS: %s", onoff_label(state->show_fps));
    draw_option_line(76, buf, state->game_sel == 0);

    if (state->show_fps) {
        snprintf(buf, sizeof(buf), "RAINBOW FPS: %s", onoff_label(state->fps_rainbow));
        draw_option_line(92, buf, state->game_sel == 1);

        snprintf(buf, sizeof(buf), "GAME VSYNC: %s", onoff_label(state->game_vsync));
        draw_option_line(108, buf, state->game_sel == 2);

        snprintf(buf, sizeof(buf), "FRAME LIMIT: %s", frame_limit_label(state->frame_limit));
        draw_option_line(124, buf, state->game_sel == 3);

        select_menu_pages_draw_frame_limit_rail(140, state->frame_limit, state->game_sel == 3);
        select_menu_font_draw_string_color(
            select_menu_pages_center_x_for_text("LEFT RIGHT OR X TO CHANGE"),
            152,
            "LEFT RIGHT OR X TO CHANGE",
            SELECT_MENU_COLOR_TEXT
        );
    } else {
        snprintf(buf, sizeof(buf), "GAME VSYNC: %s", onoff_label(state->game_vsync));
        draw_option_line(92, buf, state->game_sel == 1);

        snprintf(buf, sizeof(buf), "FRAME LIMIT: %s", frame_limit_label(state->frame_limit));
        draw_option_line(108, buf, state->game_sel == 2);

        select_menu_pages_draw_frame_limit_rail(124, state->frame_limit, state->game_sel == 2);
        select_menu_font_draw_string_color(
            select_menu_pages_center_x_for_text("LEFT RIGHT OR X TO CHANGE"),
            140,
            "LEFT RIGHT OR X TO CHANGE",
            SELECT_MENU_COLOR_TEXT
        );
    }

    select_menu_pages_draw_footer_line(166, gray);
    select_menu_pages_draw_footer_actions("CHANGE", "BACK", "CLOSE");
}
