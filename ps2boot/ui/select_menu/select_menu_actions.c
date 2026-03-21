#include "select_menu_actions_internal.h"

#include <libpad.h>

select_menu_state_t g_select_menu;

int select_menu_wrap_index(int v, int count)
{
    if (v < 0)
        return count - 1;
    if (v >= count)
        return 0;
    return v;
}

void select_menu_cycle_frame_limit(int dir)
{
    g_select_menu.frame_limit = select_menu_wrap_index(g_select_menu.frame_limit + dir, 4);
}

int select_menu_game_options_count(void)
{
    return g_select_menu.show_fps ? 3 : 2;
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
    g_select_menu.fps_rainbow = 0;
    g_select_menu.frame_limit = SELECT_MENU_FRAME_LIMIT_OFF;
    g_select_menu.game_vsync = 0;
    g_select_menu.pending_action = SELECT_MENU_ACTION_NONE;
}

int select_menu_actions_is_open(void)
{
    return g_select_menu.open;
}

void select_menu_actions_open(void)
{
    g_select_menu.open = 1;
    g_select_menu.page = SELECT_MENU_PAGE_MAIN;
    g_select_menu.pending_action = SELECT_MENU_ACTION_NONE;
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

int select_menu_actions_fps_rainbow_enabled(void)
{
    return g_select_menu.fps_rainbow;
}

int select_menu_actions_frame_limit_mode(void)
{
    return g_select_menu.frame_limit;
}

int select_menu_actions_game_vsync_enabled(void)
{
    return g_select_menu.game_vsync;
}

int select_menu_actions_pending_action(void)
{
    return g_select_menu.pending_action;
}

void select_menu_actions_clear_pending_action(void)
{
    g_select_menu.pending_action = SELECT_MENU_ACTION_NONE;
}

void select_menu_actions_handle(uint32_t pressed)
{
    if (!g_select_menu.open)
        return;

    if (pressed & PAD_SELECT) {
        g_select_menu.open = 0;
        g_select_menu.page = SELECT_MENU_PAGE_MAIN;
        return;
    }

    if (g_select_menu.page == SELECT_MENU_PAGE_MAIN) {
        select_menu_actions_handle_main(pressed);
        return;
    }

    if (g_select_menu.page == SELECT_MENU_PAGE_VIDEO ||
        g_select_menu.page == SELECT_MENU_PAGE_VIDEO_DISPLAY ||
        g_select_menu.page == SELECT_MENU_PAGE_VIDEO_ASPECT) {
        select_menu_actions_handle_video(pressed);
        return;
    }

    if (g_select_menu.page == SELECT_MENU_PAGE_GAME_OPTIONS) {
        select_menu_actions_handle_game(pressed);
        return;
    }
}
