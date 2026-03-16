#include "launcher_actions_internal.h"

#include <libpad.h>

void launcher_actions_handle_options(uint32_t pressed)
{
    if (pressed & (PAD_START | PAD_CROSS | PAD_CIRCLE))
        g_launcher.page = LAUNCHER_PAGE_MAIN;
}
