#ifndef SELECT_MENU_ACTIONS_H
#define SELECT_MENU_ACTIONS_H

#include <stdint.h>
#include "select_menu_state.h"

void select_menu_actions_init(void);
int select_menu_actions_is_open(void);
void select_menu_actions_open(void);
void select_menu_actions_close(void);
const select_menu_state_t *select_menu_actions_state(void);

int select_menu_actions_show_fps_enabled(void);
int select_menu_actions_fps_rainbow_enabled(void);
int select_menu_actions_frame_limit_mode(void);
int select_menu_actions_game_vsync_enabled(void);

int select_menu_actions_pending_action(void);
void select_menu_actions_clear_pending_action(void);

void select_menu_actions_handle(uint32_t pressed);

#endif
