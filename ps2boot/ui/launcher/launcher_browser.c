#include "launcher_browser_internal.h"

int launcher_browser_count(void)
{
    return launcher_browser_state_get()->entry_count;
}

int launcher_browser_selected(void)
{
    return launcher_browser_state_get()->selected;
}

int launcher_browser_scroll(void)
{
    return launcher_browser_state_get()->scroll;
}

const char *launcher_browser_current_path(void)
{
    const launcher_browser_state_t *state = launcher_browser_state_get();

    if (launcher_browser_is_root_path(state->current_path))
        return LAUNCHER_BROWSER_ROOT_LABEL;

    return state->current_path;
}

const launcher_browser_entry_t *launcher_browser_entry(int index)
{
    const launcher_browser_state_t *state = launcher_browser_state_get();

    if (index < 0 || index >= state->entry_count)
        return NULL;

    return &state->entries[index];
}
