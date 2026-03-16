#include "select_menu_actions_internal.h"

#include <libpad.h>

void select_menu_actions_handle_main(uint32_t pressed)
{
    if (pressed & PAD_UP)
        g_select_menu.main_sel = select_menu_wrap_index(g_select_menu.main_sel - 1, 5);
    if (pressed & PAD_DOWN)
        g_select_menu.main_sel = select_menu_wrap_index(g_select_menu.main_sel + 1, 5);

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
}
