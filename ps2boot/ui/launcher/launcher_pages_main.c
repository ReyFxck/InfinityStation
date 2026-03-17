#include <string.h>

#include "font/browser_font.h"
#include "launcher_pages_internal.h"

static unsigned main_text_width_sized(const char *text, unsigned char_w)
{
    size_t len = text ? strlen(text) : 0;
    return (unsigned)(len * (char_w + 2u));
}

static unsigned panel_center_x(unsigned panel_x, unsigned panel_w, const char *text, unsigned char_w)
{
    unsigned text_w = main_text_width_sized(text, char_w);

    if (text_w >= panel_w)
        return panel_x;

    return panel_x + (panel_w - text_w) / 2;
}

void launcher_pages_draw_main_page(const launcher_state_t *state)
{
    const uint16_t title  = 0xC638; /* cinza claro */
    const uint16_t normal = 0x39E7; /* cinza escuro */
    const uint16_t select = LAUNCHER_COLOR_HIGHLIGHT;

    const unsigned panel_x = 180;
    const unsigned panel_w = 280;

    const unsigned title_w = 13;
    const unsigned title_h = 20;

    const unsigned char_w = 14;
    const unsigned char_h = 22;

    const char *title_text = "MAIN MENU";
    const char *item0 = "BROWSE USB";
    const char *item1 = "EMBEDDED MARIO";
    const char *item2 = "OPTIONS";

    uint16_t c0 = state->main_sel == 0 ? select : normal;
    uint16_t c1 = state->main_sel == 1 ? select : normal;
    uint16_t c2 = state->main_sel == 2 ? select : normal;

    launcher_logo_draw(LAUNCHER_LOGO_X, LAUNCHER_LOGO_Y);

    browser_font_draw_string_color_sized(
        panel_center_x(panel_x, panel_w, title_text, title_w), 133,
        title_text, title, title_w, title_h
    );

    browser_font_draw_string_color_sized(
        panel_center_x(panel_x, panel_w, item0, char_w), 190,
        item0, c0, char_w, char_h
    );

    browser_font_draw_string_color_sized(
        panel_center_x(panel_x, panel_w, item1, char_w), 225,
        item1, c1, char_w, char_h
    );

    browser_font_draw_string_color_sized(
        panel_center_x(panel_x, panel_w, item2, char_w), 260,
        item2, c2, char_w, char_h
    );
}
