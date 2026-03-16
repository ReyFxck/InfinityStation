#include "launcher_pages_internal.h"

void launcher_pages_draw_main_page(const launcher_state_t *state)
{
    char label[96];

    launcher_pages_fit_text(label, sizeof(label), state->selected_label, 44);

    launcher_logo_draw(LAUNCHER_LOGO_X, LAUNCHER_LOGO_Y);

    launcher_font_draw_string_color_scaled(
        LAUNCHER_MAIN_ITEM0_X, LAUNCHER_MAIN_ITEM0_Y,
        state->main_sel == 0 ? "> BROWSE USB" : "  BROWSE USB",
        state->main_sel == 0 ? LAUNCHER_COLOR_HIGHLIGHT : LAUNCHER_COLOR_TEXT,
        3
    );

    launcher_font_draw_string_color_scaled(
        LAUNCHER_MAIN_ITEM1_X, LAUNCHER_MAIN_ITEM1_Y,
        state->main_sel == 1 ? "> EMBEDDED MARIO" : "  EMBEDDED MARIO",
        state->main_sel == 1 ? LAUNCHER_COLOR_HIGHLIGHT : LAUNCHER_COLOR_TEXT,
        3
    );

    launcher_font_draw_string_color_scaled(
        LAUNCHER_MAIN_ITEM2_X, LAUNCHER_MAIN_ITEM2_Y,
        state->main_sel == 2 ? "> OPTIONS" : "  OPTIONS",
        state->main_sel == 2 ? LAUNCHER_COLOR_HIGHLIGHT : LAUNCHER_COLOR_TEXT,
        3
    );

    launcher_font_draw_string_color_scaled(
        LAUNCHER_MAIN_INFO_X, LAUNCHER_MAIN_INFO_Y,
        "CURRENT FILE",
        LAUNCHER_COLOR_TEXT,
        2
    );

    launcher_font_draw_string_color_scaled(
        LAUNCHER_MAIN_VALUE_X, LAUNCHER_MAIN_VALUE_Y,
        label,
        LAUNCHER_COLOR_HIGHLIGHT,
        2
    );

    launcher_font_draw_string_color_scaled(
        LAUNCHER_HINT1_X, LAUNCHER_HINT1_Y,
        "X = OPEN OR START",
        LAUNCHER_COLOR_TEXT,
        2
    );

    launcher_font_draw_string_color_scaled(
        LAUNCHER_HINT2_X, LAUNCHER_HINT2_Y,
        "UP DOWN = MOVE",
        LAUNCHER_COLOR_TEXT,
        2
    );
}
