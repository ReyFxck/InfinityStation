#include "select_menu_actions_internal.h"

#include <libpad.h>
#include "ps2_video.h"

void select_menu_actions_handle_video(uint32_t pressed)
{
    int x;
    int y;

    if (g_select_menu.page == SELECT_MENU_PAGE_VIDEO) {
        if (pressed & PAD_UP)
            g_select_menu.video_sel = select_menu_wrap_index(g_select_menu.video_sel - 1, 3);
        if (pressed & PAD_DOWN)
            g_select_menu.video_sel = select_menu_wrap_index(g_select_menu.video_sel + 1, 3);

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
            g_select_menu.aspect_sel = select_menu_wrap_index(g_select_menu.aspect_sel - 1, 5);
        if (pressed & PAD_DOWN)
            g_select_menu.aspect_sel = select_menu_wrap_index(g_select_menu.aspect_sel + 1, 5);

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
