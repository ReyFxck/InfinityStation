#include "launcher_actions_internal.h"

#include <libpad.h>

void launcher_actions_handle_browser(uint32_t pressed)
{
    if (pressed & PAD_UP)
        launcher_browser_move(-1, LAUNCHER_BROWSER_ROWS);

    if (pressed & PAD_DOWN)
        launcher_browser_move(1, LAUNCHER_BROWSER_ROWS);

    if (pressed & PAD_L1)
        launcher_browser_move(-LAUNCHER_BROWSER_ROWS, LAUNCHER_BROWSER_ROWS);

    if (pressed & PAD_R1)
        launcher_browser_move(LAUNCHER_BROWSER_ROWS, LAUNCHER_BROWSER_ROWS);

    if (pressed & PAD_SELECT)
        launcher_browser_refresh();

    if (pressed & (PAD_START | PAD_CROSS)) {
        if (launcher_browser_activate(
                g_launcher.selected_path, sizeof(g_launcher.selected_path),
                g_launcher.selected_label, sizeof(g_launcher.selected_label))) {
            g_launcher.should_start_game = 1;
        }
    }

    if (pressed & PAD_CIRCLE) {
        if (!launcher_browser_go_parent())
            g_launcher.page = LAUNCHER_PAGE_MAIN;
    }
}
