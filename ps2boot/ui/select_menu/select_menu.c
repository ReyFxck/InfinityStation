#include "select_menu.h"
#include "select_menu_actions.h"
#include "select_menu_render.h"

void select_menu_init(void)
{
    select_menu_actions_init();
}

int select_menu_is_open(void)
{
    return select_menu_actions_is_open();
}

void select_menu_open(void)
{
    select_menu_actions_open();
}

void select_menu_close(void)
{
    select_menu_actions_close();
}

void select_menu_handle(uint32_t pressed)
{
    select_menu_actions_handle(pressed);
}

void select_menu_draw(void)
{
    if (!select_menu_actions_is_open())
        return;

    select_menu_render(select_menu_actions_state());
}

int select_menu_show_fps_enabled(void)
{
    return select_menu_actions_show_fps_enabled();
}

int select_menu_fps_rainbow_enabled(void)
{
    return select_menu_actions_fps_rainbow_enabled();
}

int select_menu_frame_limit_mode(void)
{
    return select_menu_actions_frame_limit_mode();
}

int select_menu_restart_game_requested(void)
{
    return select_menu_actions_pending_action() == SELECT_MENU_ACTION_RESTART;
}

void select_menu_clear_restart_game_request(void)
{
    if (select_menu_actions_pending_action() == SELECT_MENU_ACTION_RESTART)
        select_menu_actions_clear_pending_action();
}

int select_menu_exit_game_requested(void)
{
    return select_menu_actions_pending_action() == SELECT_MENU_ACTION_OPEN_LAUNCHER;
}

void select_menu_clear_exit_game_request(void)
{
    if (select_menu_actions_pending_action() == SELECT_MENU_ACTION_OPEN_LAUNCHER)
        select_menu_actions_clear_pending_action();
}
