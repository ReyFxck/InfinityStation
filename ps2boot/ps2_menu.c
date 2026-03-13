#include "ps2_menu.h"
#include "ui/select_menu/select_menu.h"

void ps2_menu_init(void)
{
    select_menu_init();
}

int ps2_menu_is_open(void)
{
    return select_menu_is_open();
}

void ps2_menu_open(void)
{
    select_menu_open();
}

void ps2_menu_close(void)
{
    select_menu_close();
}

void ps2_menu_handle(uint32_t pressed)
{
    select_menu_handle(pressed);
}

void ps2_menu_draw(void)
{
    select_menu_draw();
}
