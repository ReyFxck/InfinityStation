#include "launcher_actions_internal.h"

#include <stdio.h>
#include <string.h>

launcher_state_t g_launcher;

int launcher_actions_wrap_index(int v, int count)
{
    if (v < 0)
        return count - 1;
    if (v >= count)
        return 0;
    return v;
}

void launcher_actions_init(void)
{
    g_launcher.page = LAUNCHER_PAGE_MAIN;
    g_launcher.main_sel = 0;
    g_launcher.should_start_game = 0;
    g_launcher.selected_path[0] = '\0';
    snprintf(g_launcher.selected_label, sizeof(g_launcher.selected_label), "EMBEDDED MARIO");

    launcher_browser_init();
    launcher_browser_open("mass:/");
}

const launcher_state_t *launcher_actions_state(void)
{
    return &g_launcher;
}

int launcher_actions_should_start_game(void)
{
    return g_launcher.should_start_game;
}

void launcher_actions_clear_start_request(void)
{
    g_launcher.should_start_game = 0;
}

const char *launcher_actions_selected_path(void)
{
    return g_launcher.selected_path;
}

const char *launcher_actions_selected_label(void)
{
    return g_launcher.selected_label;
}

void launcher_actions_handle(uint32_t pressed)
{
    if (g_launcher.page == LAUNCHER_PAGE_MAIN) {
        launcher_actions_handle_main(pressed);
        return;
    }

    if (g_launcher.page == LAUNCHER_PAGE_BROWSER) {
        launcher_actions_handle_browser(pressed);
        return;
    }

    launcher_actions_handle_options(pressed);
}
