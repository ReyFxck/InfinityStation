#ifndef LAUNCHER_ACTIONS_H
#define LAUNCHER_ACTIONS_H

#include <stdint.h>
#include "launcher_state.h"

void launcher_actions_init(void);
void launcher_actions_handle(uint32_t pressed);
const launcher_state_t *launcher_actions_state(void);

int launcher_actions_should_start_game(void);
void launcher_actions_clear_start_request(void);

const char *launcher_actions_selected_path(void);
const char *launcher_actions_selected_label(void);

#endif
