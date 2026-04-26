#include "actions_internal.h"

void launcher_actions_handle_options(uint32_t pressed)
{
    if (pressed & (PAD_START | PAD_CROSS | PAD_CIRCLE))
        launcher_state_mut()->page = LAUNCHER_PAGE_MAIN;
}
