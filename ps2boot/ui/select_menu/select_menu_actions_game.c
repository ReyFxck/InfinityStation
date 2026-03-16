#include "select_menu_actions_internal.h"

#include <libpad.h>

void select_menu_actions_handle_game(uint32_t pressed)
{
    int count = select_menu_game_options_count();

    if (pressed & PAD_UP)
        g_select_menu.game_sel = select_menu_wrap_index(g_select_menu.game_sel - 1, count);
    if (pressed & PAD_DOWN)
        g_select_menu.game_sel = select_menu_wrap_index(g_select_menu.game_sel + 1, count);

    if (g_select_menu.game_sel == 0) {
        if ((pressed & PAD_LEFT) || (pressed & PAD_RIGHT) || (pressed & PAD_START) || (pressed & PAD_CROSS)) {
            g_select_menu.show_fps = !g_select_menu.show_fps;
            if (!g_select_menu.show_fps) {
                g_select_menu.fps_rainbow = 0;
                if (g_select_menu.game_sel >= select_menu_game_options_count())
                    g_select_menu.game_sel = select_menu_game_options_count() - 1;
            }
        }
    } else if (g_select_menu.show_fps && g_select_menu.game_sel == 1) {
        if ((pressed & PAD_LEFT) || (pressed & PAD_RIGHT) || (pressed & PAD_START) || (pressed & PAD_CROSS))
            g_select_menu.fps_rainbow = !g_select_menu.fps_rainbow;
    } else if ((!g_select_menu.show_fps && g_select_menu.game_sel == 1) ||
               ( g_select_menu.show_fps && g_select_menu.game_sel == 2)) {
        if (pressed & PAD_LEFT)
            select_menu_cycle_frame_limit(-1);
        if (pressed & PAD_RIGHT)
            select_menu_cycle_frame_limit(1);
        if ((pressed & PAD_START) || (pressed & PAD_CROSS))
            select_menu_cycle_frame_limit(1);
    } else {
        if (pressed & (PAD_START | PAD_CROSS))
            g_select_menu.page = SELECT_MENU_PAGE_MAIN;
    }

    if (pressed & PAD_CIRCLE)
        g_select_menu.page = SELECT_MENU_PAGE_MAIN;
}
