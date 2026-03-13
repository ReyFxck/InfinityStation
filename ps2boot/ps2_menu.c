#include "ps2_menu.h"

#include <libpad.h>

#include "ps2_video.h"

typedef struct
{
    int open;
    int page;
    int main_sel;
    int video_sel;
    int aspect_sel;
} ps2_menu_state_t;

static ps2_menu_state_t g_menu;

static int wrap_index(int v, int count)
{
    if (v < 0)
        return count - 1;
    if (v >= count)
        return 0;
    return v;
}

void ps2_menu_init(void)
{
    g_menu.open = 0;
    g_menu.page = PS2_MENU_MAIN;
    g_menu.main_sel = 0;
    g_menu.video_sel = 0;
    g_menu.aspect_sel = 0;
}

int ps2_menu_is_open(void)
{
    return g_menu.open;
}

void ps2_menu_open(void)
{
    g_menu.open = 1;
    g_menu.page = PS2_MENU_MAIN;
}

void ps2_menu_close(void)
{
    g_menu.open = 0;
}

void ps2_menu_handle(uint32_t pressed)
{
    int x, y;

    if (!g_menu.open)
        return;

    if (pressed & PAD_SELECT) {
        g_menu.open = 0;
        g_menu.page = PS2_MENU_MAIN;
        return;
    }

    if (g_menu.page == PS2_MENU_MAIN) {
        if (pressed & PAD_UP)
            g_menu.main_sel = wrap_index(g_menu.main_sel - 1, 2);
        if (pressed & PAD_DOWN)
            g_menu.main_sel = wrap_index(g_menu.main_sel + 1, 2);

        if (pressed & (PAD_START | PAD_CROSS)) {
            if (g_menu.main_sel == 0) {
                g_menu.open = 0;
            } else {
                g_menu.page = PS2_MENU_VIDEO;
                g_menu.video_sel = 0;
            }
        }
        return;
    }

    if (g_menu.page == PS2_MENU_VIDEO) {
        if (pressed & PAD_UP)
            g_menu.video_sel = wrap_index(g_menu.video_sel - 1, 3);
        if (pressed & PAD_DOWN)
            g_menu.video_sel = wrap_index(g_menu.video_sel + 1, 3);

        if (pressed & (PAD_START | PAD_CROSS)) {
            if (g_menu.video_sel == 0) {
                g_menu.page = PS2_MENU_VIDEO_DISPLAY;
            } else if (g_menu.video_sel == 1) {
                g_menu.page = PS2_MENU_VIDEO_ASPECT;
                g_menu.aspect_sel = ps2_video_get_aspect();
            } else {
                g_menu.page = PS2_MENU_MAIN;
            }
        }

        if (pressed & PAD_CIRCLE)
            g_menu.page = PS2_MENU_MAIN;

        return;
    }

    if (g_menu.page == PS2_MENU_VIDEO_DISPLAY) {
        ps2_video_get_offsets(&x, &y);

        if (pressed & PAD_LEFT)  x -= 8;
        if (pressed & PAD_RIGHT) x += 8;
        if (pressed & PAD_UP)    y -= 8;
        if (pressed & PAD_DOWN)  y += 8;

        ps2_video_set_offsets(x, y);

        if (pressed & (PAD_START | PAD_CROSS | PAD_CIRCLE))
            g_menu.page = PS2_MENU_VIDEO;

        return;
    }

    if (g_menu.page == PS2_MENU_VIDEO_ASPECT) {
        if (pressed & PAD_UP)
            g_menu.aspect_sel = wrap_index(g_menu.aspect_sel - 1, 5);
        if (pressed & PAD_DOWN)
            g_menu.aspect_sel = wrap_index(g_menu.aspect_sel + 1, 5);

        if (pressed & (PAD_START | PAD_CROSS)) {
            if (g_menu.aspect_sel == 0)
                ps2_video_set_aspect(PS2_ASPECT_4_3);
            else if (g_menu.aspect_sel == 1)
                ps2_video_set_aspect(PS2_ASPECT_16_9);
            else if (g_menu.aspect_sel == 2)
                ps2_video_set_aspect(PS2_ASPECT_FULL);
            else if (g_menu.aspect_sel == 3)
                ps2_video_set_aspect(PS2_ASPECT_PIXEL);
            else
                g_menu.page = PS2_MENU_VIDEO;
        }

        if (pressed & PAD_CIRCLE)
            g_menu.page = PS2_MENU_VIDEO;

        return;
    }
}

void ps2_menu_draw(void)
{
    if (!g_menu.open)
        return;

    ps2_video_draw_menu(g_menu.page, g_menu.main_sel, g_menu.video_sel, g_menu.aspect_sel);
}
