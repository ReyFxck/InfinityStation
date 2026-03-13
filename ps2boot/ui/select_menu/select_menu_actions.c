#include "select_menu_actions.h"

#include <libpad.h>
#include "../../ps2_video.h"

static select_menu_state_t g_select_menu;

static int wrap_index(int v, int count)
{
    if (v < 0)
        return count - 1;
    if (v >= count)
        return 0;
    return v;
}

void select_menu_actions_init(void)
{
    g_select_menu.open = 0;
    g_select_menu.page = SELECT_MENU_PAGE_MAIN;
    g_select_menu.main_sel = 0;
    g_select_menu.video_sel = 0;
    g_select_menu.aspect_sel = 0;
}

int select_menu_actions_is_open(void)
{
    return g_select_menu.open;
}

void select_menu_actions_open(void)
{
    g_select_menu.open = 1;
    g_select_menu.page = SELECT_MENU_PAGE_MAIN;
}

void select_menu_actions_close(void)
{
    g_select_menu.open = 0;
}

const select_menu_state_t *select_menu_actions_state(void)
{
    return &g_select_menu;
}

void select_menu_actions_handle(uint32_t pressed)
{
    int x;
    int y;

    if (!g_select_menu.open)
        return;

    if (pressed & PAD_SELECT) {
        g_select_menu.open = 0;
        g_select_menu.page = SELECT_MENU_PAGE_MAIN;
        return;
    }

    if (g_select_menu.page == SELECT_MENU_PAGE_MAIN) {
        if (pressed & PAD_UP)
            g_select_menu.main_sel = wrap_index(g_select_menu.main_sel - 1, 2);
        if (pressed & PAD_DOWN)
            g_select_menu.main_sel = wrap_index(g_select_menu.main_sel + 1, 2);

        if (pressed & (PAD_START | PAD_CROSS)) {
            if (g_select_menu.main_sel == 0) {
                g_select_menu.open = 0;
            } else {
                g_select_menu.page = SELECT_MENU_PAGE_VIDEO;
                g_select_menu.video_sel = 0;
            }
        }
        return;
    }

    if (g_select_menu.page == SELECT_MENU_PAGE_VIDEO) {
        if (pressed & PAD_UP)
            g_select_menu.video_sel = wrap_index(g_select_menu.video_sel - 1, 3);
        if (pressed & PAD_DOWN)
            g_select_menu.video_sel = wrap_index(g_select_menu.video_sel + 1, 3);

        if (pressed & (PAD_START | PAD_CROSS)) {
            if (g_select_menu.video_sel == 0) {
                g_select_menu.page = SELECT_MENU_PAGE_VIDEO_DISPLAY;
            } else if (g_select_menu.video_sel == 1) {
                g_select_menu.page = SELECT_MENU_PAGE_VIDEO_ASPECT;
                g_select_menu.aspect_sel = ps2_video_get_aspect();
            } else {
                g_select_menu.page = SELECT_MENU_PAGE_MAIN;
            }
        }

        if (pressed & PAD_CIRCLE)
            g_select_menu.page = SELECT_MENU_PAGE_MAIN;
        return;
    }

    if (g_select_menu.page == SELECT_MENU_PAGE_VIDEO_DISPLAY) {
        ps2_video_get_offsets(&x, &y);

        if (pressed & PAD_LEFT)
            x -= 8;
        if (pressed & PAD_RIGHT)
            x += 8;
        if (pressed & PAD_UP)
            y -= 8;
        if (pressed & PAD_DOWN)
            y += 8;

        ps2_video_set_offsets(x, y);

        if (pressed & (PAD_START | PAD_CROSS | PAD_CIRCLE))
            g_select_menu.page = SELECT_MENU_PAGE_VIDEO;
        return;
    }

    if (g_select_menu.page == SELECT_MENU_PAGE_VIDEO_ASPECT) {
        if (pressed & PAD_UP)
            g_select_menu.aspect_sel = wrap_index(g_select_menu.aspect_sel - 1, 5);
        if (pressed & PAD_DOWN)
            g_select_menu.aspect_sel = wrap_index(g_select_menu.aspect_sel + 1, 5);

        if (pressed & (PAD_START | PAD_CROSS)) {
            if (g_select_menu.aspect_sel == 0)
                ps2_video_set_aspect(PS2_ASPECT_4_3);
            else if (g_select_menu.aspect_sel == 1)
                ps2_video_set_aspect(PS2_ASPECT_16_9);
            else if (g_select_menu.aspect_sel == 2)
                ps2_video_set_aspect(PS2_ASPECT_FULL);
            else if (g_select_menu.aspect_sel == 3)
                ps2_video_set_aspect(PS2_ASPECT_PIXEL);

            g_select_menu.page = SELECT_MENU_PAGE_VIDEO;
        }

        if (pressed & PAD_CIRCLE)
            g_select_menu.page = SELECT_MENU_PAGE_VIDEO;

        return;
    }
}
