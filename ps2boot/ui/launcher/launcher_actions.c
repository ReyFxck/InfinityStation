#include "launcher_actions.h"

#include <libpad.h>
#include <stdio.h>
#include <string.h>
#include "launcher_browser.h"
#include "launcher_theme.h"

static launcher_state_t g_launcher;

static int wrap_index(int v, int count)
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
        if (pressed & PAD_UP)
            g_launcher.main_sel = wrap_index(g_launcher.main_sel - 1, 3);
        if (pressed & PAD_DOWN)
            g_launcher.main_sel = wrap_index(g_launcher.main_sel + 1, 3);

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
        return;
    }

    if (g_launcher.page == LAUNCHER_PAGE_BROWSER) {
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
        return;
    }

    if (g_launcher.page == LAUNCHER_PAGE_OPTIONS) {
        if (pressed & (PAD_START | PAD_CROSS | PAD_CIRCLE))
            g_launcher.page = LAUNCHER_PAGE_MAIN;
        return;
    }
}
