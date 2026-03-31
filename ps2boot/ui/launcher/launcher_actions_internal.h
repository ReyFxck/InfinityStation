#ifndef LAUNCHER_ACTIONS_INTERNAL_H
#define LAUNCHER_ACTIONS_INTERNAL_H

#include <stdint.h>
#include <libpad.h>

#include "launcher_actions.h"
#include "launcher_state.h"
#include "launcher_browser.h"
#include "launcher_theme.h"

int launcher_actions_wrap_index(int v, int count);
void launcher_actions_handle_main(uint32_t pressed);
void launcher_actions_handle_browser(uint32_t pressed);
void launcher_actions_handle_options(uint32_t pressed);

#endif
