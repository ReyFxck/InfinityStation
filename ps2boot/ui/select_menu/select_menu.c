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
