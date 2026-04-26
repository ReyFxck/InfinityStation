#include "pages_internal.h"

#include <string.h>

static void draw_icon_bitmap(unsigned x, unsigned y, const char **rows, unsigned h, uint16_t color)
{
    unsigned yy;
    unsigned xx;

    for (yy = 0; yy < h; yy++) {
        unsigned w = (unsigned)strlen(rows[yy]);
        for (xx = 0; xx < w; xx++) {
            if (rows[yy][xx] == '#')
                ps2_video_menu_put_pixel(x + xx, y + yy, color);
        }
    }
}

void select_menu_pages_draw_icon_cross(unsigned x, unsigned y)
{
    static const char *icon[] = {
        "#.....#",
        ".#...#.",
        "..#.#..",
        "...#...",
        "..#.#..",
        ".#...#.",
        "#.....#"
    };

    draw_icon_bitmap(x, y, icon, 7, 0x001F);
}

void select_menu_pages_draw_icon_circle(unsigned x, unsigned y)
{
    static const char *icon[] = {
        "...###...",
        "..#...#..",
        ".#.....#.",
        "#.......#",
        "#.......#",
        "#.......#",
        ".#.....#.",
        "..#...#..",
        "...###..."
    };

    draw_icon_bitmap(x, y, icon, 9, 0xF800);
}

void select_menu_pages_draw_icon_select(unsigned x, unsigned y)
{
    static const char *icon[] = {
        ".#########.",
        "#.........#",
        "#.........#",
        "#.........#",
        "#.........#",
        "#.........#",
        ".#########."
    };

    draw_icon_bitmap(x, y, icon, 7, 0x6B2D);
}
