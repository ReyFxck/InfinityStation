#ifndef SELECT_MENU_ACTIONS_INTERNAL_H
#define SELECT_MENU_ACTIONS_INTERNAL_H

#include "select_menu_actions.h"

extern select_menu_state_t g_select_menu;

int select_menu_wrap_index(int v, int count);
void select_menu_cycle_frame_limit(int dir);
int select_menu_game_options_count(void);

void select_menu_actions_handle_main(uint32_t pressed);
void select_menu_actions_handle_video(uint32_t pressed);
void select_menu_actions_handle_game(uint32_t pressed);

#endif
