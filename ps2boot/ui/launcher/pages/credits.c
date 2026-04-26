#include <string.h>
#include "../font/browser_font.h"
#include "pages_internal.h"

extern const unsigned int launcher_logo_width;
extern const unsigned int launcher_logo_height;

static unsigned credits_text_width_sized(const char *text, unsigned char_w)
{
    size_t len = text ? strlen(text) : 0;
    return (unsigned)(len * (char_w + 2u));
}

static unsigned credits_center_x_in_box(unsigned box_x, unsigned box_w,
                                        const char *text, unsigned char_w)
{
    unsigned text_w = credits_text_width_sized(text, char_w);
    if (text_w >= box_w)
        return box_x;
    return box_x + (box_w - text_w) / 2;
}

static unsigned credits_screen_center_x(unsigned screen_w, unsigned obj_w)
{
    if (obj_w >= screen_w)
        return 0;
    return (screen_w - obj_w) / 2;
}

static void credits_draw_center_line(unsigned box_x, unsigned box_w,
                                     unsigned y, const char *text,
                                     uint16_t color,
                                     unsigned char_w, unsigned char_h)
{
    unsigned x = credits_center_x_in_box(box_x, box_w, text, char_w);

    browser_font_draw_string_color_sized(
        x, y, text, color, char_w, char_h
    );
}

void launcher_pages_draw_credits_page(void)
{
    static unsigned scroll_tick = 0;

    static const char *lines[] = {
        "THANK YOU FOR TRYING THIS PROJECT",
        "THIS IS A HOBBY PROJECT MADE FOR FUN",
        "I HOPE YOU ENJOY IT",
        "",
        "- CORE -",
        "THANKS TO THE LIBRETRO COMMUNITY",
        "BASED ON SNES9X2005, ADAPTED FOR PS2",
        "",
        "- IDEAS -",
        "SNESSTATION / PGEN DEVELOPERS",
        "LIBRETRO / SNESTICLE",
        "",
        "- LEGACY CREDITS -",
        "A. HIRYU - UI IDEAS",
        "ARAGON (PGEN) - UI IDEAS / FOLDER FIXES",
        "ISRA (WLAUNCHELF ISR) - FOLDER / SELECTION FIXES",
        "PS2SDK / PS2DEV - PS2 ELF DEVELOPMENT",
        "OPL TEAM - IOP METHOD IDEAS",
        "PICODRIVE - AUDSRV LOGIC IDEAS",
        "SNESTICLE / IADDIS / FORKS - IDEAS / INSPIRATION",
        "MINIZ - ZIP SUPPORT FOR ROM FILES",
        "AND OTHERS I MAY HAVE FORGOTTEN",
        "",
        "THANK YOU FOR PLAYING"
    };

    const int line_count = (int)(sizeof(lines) / sizeof(lines[0]));
    const int gap_lines = 4;
    const int total_slots = line_count + gap_lines;

    const int panel_x = 95;
    const int panel_y = 95;
    const int panel_w = 450;
    const int panel_h = 285;

    const int title_y = panel_y + 10;
    const int view_top = panel_y + 46;
    const int view_bottom = panel_y + panel_h - 18;
    const int view_mid = (view_top + view_bottom) / 2;
    const int line_gap = 17;
    const int total_height = total_slots * line_gap;

    const uint16_t title_color = 0x2411; /* cyan-ish */
    const uint16_t body_color  = 0x39E7; /* dark gray */
    const uint16_t head_color  = 0x7053; /* purple */

    const unsigned title_w = 13;
    const unsigned title_h = 20;
    const unsigned body_w = 7;
    const unsigned body_h = 12;
    const unsigned head_w = 9;
    const unsigned head_h = 14;

    int i;
    int offset;
    int start_y;

    scroll_tick++;
    offset = (int)(((scroll_tick * 4u)) % (unsigned)total_height);
    start_y = view_mid - offset;

    launcher_logo_draw(credits_screen_center_x(640, launcher_logo_width), 18);

    credits_draw_center_line(panel_x, panel_w, (unsigned)title_y,
                             "CREDITS", title_color, title_w, title_h);

    for (i = 0; i < total_slots * 2; ++i) {
        int slot = i % total_slots;
        int y = start_y + i * line_gap;
        const char *text;
        uint16_t color = body_color;
        unsigned cw = body_w;
        unsigned ch = body_h;

        if (slot >= line_count)
            continue;

        text = lines[slot];
        if (!text || !text[0])
            continue;

        if (y < view_top || y > view_bottom)
            continue;

        if (text[0] == '-') {
            color = head_color;
            cw = head_w;
            ch = head_h;
        }

        credits_draw_center_line(panel_x, panel_w, (unsigned)y,
                                 text, color, cw, ch);
    }
}
