#ifndef SELECT_MENU_ACTIONS_H
#define SELECT_MENU_ACTIONS_H

#include <stdint.h>
#include "select_menu_state.h"

void select_menu_actions_init(void);
int select_menu_actions_is_open(void);
void select_menu_actions_open(void);
void select_menu_actions_close(void);
void select_menu_actions_handle(uint32_t pressed);
const select_menu_state_t *select_menu_actions_state(void);

#endif
