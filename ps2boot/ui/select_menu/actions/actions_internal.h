#ifndef SELECT_MENU_ACTIONS_INTERNAL_H
#define SELECT_MENU_ACTIONS_INTERNAL_H

#include <stdint.h>
#include <libpad.h>

#include "actions.h"

int select_menu_wrap_index(int v, int count);
void select_menu_cycle_frame_limit(int dir);
int select_menu_game_options_count(void);
void select_menu_actions_handle_main(uint32_t pressed);
void select_menu_actions_handle_video(uint32_t pressed);
void select_menu_actions_handle_game(uint32_t pressed);

#endif
