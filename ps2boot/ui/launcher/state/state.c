#include "state.h"

#include <stdio.h>
#include <string.h>

static launcher_state_t g_launcher_state;

void launcher_state_reset(void)
{
    memset(&g_launcher_state, 0, sizeof(g_launcher_state));
    g_launcher_state.page = LAUNCHER_PAGE_MAIN;
    g_launcher_state.main_sel = 0;
    g_launcher_state.should_start_game = 0;
    g_launcher_state.selected_path[0] = '\0';
    g_launcher_state.selected_label[0] = '\0';
}

const launcher_state_t *launcher_state_get(void)
{
    return &g_launcher_state;
}

launcher_state_t *launcher_state_mut(void)
{
    return &g_launcher_state;
}
