#include "pages_internal.h"

void launcher_pages_draw_options_page(void)
{
    launcher_font_draw_string_color_scaled(
        LAUNCHER_OPTIONS_TITLE_X, LAUNCHER_OPTIONS_TITLE_Y,
        "OPTIONS",
        LAUNCHER_COLOR_TEXT,
        4
    );

    launcher_font_draw_string_color_scaled(
        LAUNCHER_OPTIONS_INFO_X, LAUNCHER_OPTIONS_INFO_Y,
        "COMING SOON",
        LAUNCHER_COLOR_HIGHLIGHT,
        3
    );

    launcher_font_draw_string_color_scaled(
        LAUNCHER_OPTIONS_BACK_X, LAUNCHER_OPTIONS_BACK_Y,
        "> BACK",
        LAUNCHER_COLOR_TEXT,
        3
    );

    launcher_font_draw_string_color_scaled(
        54, 404,
        "X OR O = BACK",
        LAUNCHER_COLOR_TEXT,
        2
    );
}
