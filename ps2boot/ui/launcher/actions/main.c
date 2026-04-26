#include "actions_internal.h"

#include <stdio.h>

void launcher_actions_handle_main(uint32_t pressed)
{
    launcher_state_t *state = launcher_state_mut();

    if (pressed & PAD_UP)
        state->main_sel = launcher_actions_wrap_index(state->main_sel - 1, 3);

    if (pressed & PAD_DOWN)
        state->main_sel = launcher_actions_wrap_index(state->main_sel + 1, 3);

    if (pressed & (PAD_START | PAD_CROSS)) {
        if (state->main_sel == 0) {
            state->page = LAUNCHER_PAGE_BROWSER;
        } else if (state->main_sel == 1) {
            state->page = LAUNCHER_PAGE_OPTIONS;
        } else {
            state->page = LAUNCHER_PAGE_CREDITS;
        }
    }
}
