#include "ps2_video_internal.h"

static void ps2_video_menu_clear_visible(void)
{
    unsigned y;

    for (y = 0; y < 224u; y++) {
        memset(&g_upload[y * PS2_VIDEO_TEX_WIDTH],
               0,
               256u * sizeof(g_upload[0]));
    }
}


void ps2_video_menu_put_pixel_store(unsigned x, unsigned y, uint16_t color)
{
    if (x >= 256 || y >= 224)
        return;

    g_upload[y * PS2_VIDEO_TEX_WIDTH + x] = ps2_video_convert_rgb565(color);
}

void ps2_video_menu_put_pixel_raw(unsigned x, unsigned y, uint16_t color)
{
    if (x >= 256 || y >= 224)
        return;

    g_upload[y * PS2_VIDEO_TEX_WIDTH + x] = color;
}

void ps2_video_menu_put_pixel(unsigned x, unsigned y, uint16_t color)
{
    ps2_video_menu_put_pixel_store(x, y, color);
}

void ps2_video_menu_begin_frame(void)
{
    ps2_video_menu_clear_visible();
    menu_tint_blue();
}

void ps2_video_menu_end_frame(void)
{
    ps2_video_upload_and_draw_bound(256, 224, 1);
}

void ps2_video_draw_menu(int page, int main_sel, int video_sel, int aspect_sel)
{
    char buf_x[40];
    char buf_y[40];
    uint16_t white  = 0xFFFF;
    uint16_t yellow = 0x83FF;

    ps2_video_menu_clear_visible();
    menu_tint_blue();

    if (page == PS2_MENU_MAIN) {
        dbg_draw_string_color(115, 18, "MENU", white);

        dbg_draw_string_color(104, 92,
                              main_sel == 0 ? "> RESUME" : "  RESUME",
                              main_sel == 0 ? yellow : white);

        dbg_draw_string_color(116, 110,
                              main_sel == 1 ? "> VIDEO" : "  VIDEO",
                              main_sel == 1 ? yellow : white);

        dbg_draw_string_color(8, 194, "SELECT CLOSE", white);
    }
    else if (page == PS2_MENU_VIDEO) {
        dbg_draw_string_color(110, 18, "VIDEO", white);

        dbg_draw_string_color(62, 88,
                              video_sel == 0 ? "> DISPLAY POSITION" : "  DISPLAY POSITION",
                              video_sel == 0 ? yellow : white);

        dbg_draw_string_color(80, 106,
                              video_sel == 1 ? "> ASPECT RATIO" : "  ASPECT RATIO",
                              video_sel == 1 ? yellow : white);

        dbg_draw_string_color(116, 124,
                              video_sel == 2 ? "> BACK" : "  BACK",
                              video_sel == 2 ? yellow : white);

        dbg_draw_string_color(8, 180, "CROSS = OPEN", white);
        dbg_draw_string_color(8, 194, "SELECT CLOSE", white);
    }
    else if (page == PS2_MENU_VIDEO_DISPLAY) {
        dbg_draw_string_color(74, 18, "DISPLAY POSITION", white);
        dbg_draw_string_color(80, 74, "D-PAD MOVES OUTPUT", white);

        snprintf(buf_x, sizeof(buf_x), "X = %d", g_video_off_x);
        snprintf(buf_y, sizeof(buf_y), "Y = %d", g_video_off_y);

        dbg_draw_string_color(104, 94, buf_x, yellow);
        dbg_draw_string_color(104, 108, buf_y, yellow);

        dbg_draw_string_color(47, 130, "CROSS START CIRCLE = BACK", white);
        dbg_draw_string_color(8, 194, "SELECT CLOSE", white);
    }
    else {
        dbg_draw_string_color(86, 18, "ASPECT RATIO", white);

        dbg_draw_string_color(118, 74,
                              aspect_sel == 0 ? "> 4:3" : "  4:3",
                              aspect_sel == 0 ? yellow : white);

        dbg_draw_string_color(114, 88,
                              aspect_sel == 1 ? "> 16:9" : "  16:9",
                              aspect_sel == 1 ? yellow : white);

        dbg_draw_string_color(74, 102,
                              aspect_sel == 2 ? "> FULL SCREEN" : "  FULL SCREEN",
                              aspect_sel == 2 ? yellow : white);

        dbg_draw_string_color(68, 116,
                              aspect_sel == 3 ? "> PIXEL PERFECT" : "  PIXEL PERFECT",
                              aspect_sel == 3 ? yellow : white);

        dbg_draw_string_color(114, 130,
                              aspect_sel == 4 ? "> BACK" : "  BACK",
                              aspect_sel == 4 ? yellow : white);

        dbg_draw_string_color(38, 170, "CROSS START = APPLY", white);
        dbg_draw_string_color(8, 194, "SELECT CLOSE", white);
    }

    ps2_video_upload_and_draw_bound(256, 224, 1);
}

uint16_t ps2_video_menu_get_pixel(unsigned x, unsigned y)
{
    if (x >= 256 || y >= 224)
        return 0;

    return g_upload[y * PS2_VIDEO_TEX_WIDTH + x];
}
