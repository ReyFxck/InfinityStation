#include "launcher_actions_internal.h"

#include <libpad.h>
#include <stdio.h>

void launcher_actions_handle_main(uint32_t pressed)
{
    if (pressed & PAD_UP)
        g_launcher.main_sel = launcher_actions_wrap_index(g_launcher.main_sel - 1, 3);

    if (pressed & PAD_DOWN)
        g_launcher.main_sel = launcher_actions_wrap_index(g_launcher.main_sel + 1, 3);

    if (pressed & (PAD_START | PAD_CROSS)) {
        if (g_launcher.main_sel == 0) {
            g_launcher.page = LAUNCHER_PAGE_BROWSER;
        } else if (g_launcher.main_sel == 1) {
            g_launcher.selected_path[0] = '\0';
            snprintf(g_launcher.selected_label, sizeof(g_launcher.selected_label), "EMBEDDED MARIO");
            g_launcher.should_start_game = 1;
        } else {
            g_launcher.page = LAUNCHER_PAGE_OPTIONS;
        }
    }
}
