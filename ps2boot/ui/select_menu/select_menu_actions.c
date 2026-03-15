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

static void cycle_frame_limit(int dir)
{
    g_select_menu.frame_limit = wrap_index(g_select_menu.frame_limit + dir, 4);
}

void select_menu_actions_init(void)
{
    g_select_menu.open = 0;
    g_select_menu.page = SELECT_MENU_PAGE_MAIN;
    g_select_menu.main_sel = 0;
    g_select_menu.video_sel = 0;
    g_select_menu.aspect_sel = 0;
    g_select_menu.game_sel = 0;
    g_select_menu.show_fps = 0;
    g_select_menu.frame_limit = SELECT_MENU_FRAME_LIMIT_AUTO;
    g_select_menu.request_restart_game = 0;
    g_select_menu.request_exit_game = 0;
}

int select_menu_actions_is_open(void)
{
    return g_select_menu.open;
}

void select_menu_actions_open(void)
{
    g_select_menu.open = 1;
    g_select_menu.page = SELECT_MENU_PAGE_MAIN;
    g_select_menu.request_restart_game = 0;
    g_select_menu.request_exit_game = 0;
}

void select_menu_actions_close(void)
{
    g_select_menu.open = 0;
}

const select_menu_state_t *select_menu_actions_state(void)
{
    return &g_select_menu;
}

int select_menu_actions_show_fps_enabled(void)
{
    return g_select_menu.show_fps;
}

int select_menu_actions_frame_limit_mode(void)
{
    return g_select_menu.frame_limit;
}

int select_menu_actions_restart_game_requested(void)
{
    return g_select_menu.request_restart_game;
}

void select_menu_actions_clear_restart_game_request(void)
{
    g_select_menu.request_restart_game = 0;
}

int select_menu_actions_exit_game_requested(void)
{
    return g_select_menu.request_exit_game;
}

void select_menu_actions_clear_exit_game_request(void)
{
    g_select_menu.request_exit_game = 0;
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
            g_select_menu.main_sel = wrap_index(g_select_menu.main_sel - 1, 5);
        if (pressed & PAD_DOWN)
            g_select_menu.main_sel = wrap_index(g_select_menu.main_sel + 1, 5);

        if (pressed & (PAD_START | PAD_CROSS)) {
            if (g_select_menu.main_sel == 0) {
                g_select_menu.open = 0;
            } else if (g_select_menu.main_sel == 1) {
                g_select_menu.request_restart_game = 1;
                g_select_menu.open = 0;
                g_select_menu.page = SELECT_MENU_PAGE_MAIN;
            } else if (g_select_menu.main_sel == 2) {
                g_select_menu.page = SELECT_MENU_PAGE_VIDEO;
                g_select_menu.video_sel = 0;
            } else if (g_select_menu.main_sel == 3) {
                g_select_menu.page = SELECT_MENU_PAGE_GAME_OPTIONS;
                g_select_menu.game_sel = 0;
            } else {
                g_select_menu.request_exit_game = 1;
                g_select_menu.open = 0;
                g_select_menu.page = SELECT_MENU_PAGE_MAIN;
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

    if (g_select_menu.page == SELECT_MENU_PAGE_GAME_OPTIONS) {
        if (pressed & PAD_UP)
            g_select_menu.game_sel = wrap_index(g_select_menu.game_sel - 1, 3);
        if (pressed & PAD_DOWN)
            g_select_menu.game_sel = wrap_index(g_select_menu.game_sel + 1, 3);

        if (g_select_menu.game_sel == 0) {
            if ((pressed & PAD_LEFT) || (pressed & PAD_RIGHT) || (pressed & PAD_START) || (pressed & PAD_CROSS))
                g_select_menu.show_fps = !g_select_menu.show_fps;
        } else if (g_select_menu.game_sel == 1) {
            if (pressed & PAD_LEFT)
                cycle_frame_limit(-1);
            if (pressed & PAD_RIGHT)
                cycle_frame_limit(1);
            if ((pressed & PAD_START) || (pressed & PAD_CROSS))
                cycle_frame_limit(1);
        } else {
            if (pressed & (PAD_START | PAD_CROSS))
                g_select_menu.page = SELECT_MENU_PAGE_MAIN;
        }

        if (pressed & PAD_CIRCLE)
            g_select_menu.page = SELECT_MENU_PAGE_MAIN;

        return;
    }
}
