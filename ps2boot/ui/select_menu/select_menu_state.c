#include "select_menu_state.h"

static select_menu_state_t g_select_menu_state;

void select_menu_state_reset(void)
{
    g_select_menu_state.open = 0;
    g_select_menu_state.page = SELECT_MENU_PAGE_MAIN;
    g_select_menu_state.main_sel = 0;
    g_select_menu_state.video_sel = 0;
    g_select_menu_state.aspect_sel = 0;
    g_select_menu_state.game_sel = 0;
    g_select_menu_state.show_fps = 0;
    g_select_menu_state.fps_rainbow = 0;
    g_select_menu_state.frame_limit = SELECT_MENU_FRAME_LIMIT_OFF;
    g_select_menu_state.game_vsync = 0;
    g_select_menu_state.game_reduce_slowdown = 0;
    g_select_menu_state.game_reduce_flicker = 0;
    g_select_menu_state.pending_action = SELECT_MENU_ACTION_NONE;
}

const select_menu_state_t *select_menu_state_get(void)
{
    return &g_select_menu_state;
}

select_menu_state_t *select_menu_state_mut(void)
{
    return &g_select_menu_state;
}
