#include "browser_internal.h"

static launcher_browser_state_t g_launcher_browser_state;

void launcher_browser_state_reset(void)
{
    memset(&g_launcher_browser_state, 0, sizeof(g_launcher_browser_state));
    g_launcher_browser_state.current_path[0] = '\0';
    g_launcher_browser_state.scan_done = 1;
}

const launcher_browser_state_t *launcher_browser_state_get(void)
{
    return &g_launcher_browser_state;
}

launcher_browser_state_t *launcher_browser_state_mut(void)
{
    return &g_launcher_browser_state;
}

int launcher_browser_is_root_path(const char *path)
{
    return !path || !path[0];
}

void launcher_browser_close_scan_dir(void)
{
    launcher_browser_state_t *state = launcher_browser_state_mut();

    if (state->scan_dir) {
        closedir(state->scan_dir);
        state->scan_dir = NULL;
    }
}

void launcher_browser_clear_entries(void)
{
    launcher_browser_state_t *state = launcher_browser_state_mut();

    free(state->entries);
    state->entries = NULL;
    state->entry_count = 0;
    state->entry_capacity = 0;
    state->selected = 0;
    state->scroll = 0;
    state->last_error = 0;
}

int launcher_browser_ensure_capacity(int need)
{
    launcher_browser_state_t *state = launcher_browser_state_mut();
    launcher_browser_entry_t *tmp;
    int new_capacity;

    if (need <= state->entry_capacity)
        return 1;

    new_capacity = state->entry_capacity;
    if (new_capacity < LAUNCHER_BROWSER_CAPACITY_GROW)
        new_capacity = LAUNCHER_BROWSER_CAPACITY_GROW;

    while (new_capacity < need)
        new_capacity += LAUNCHER_BROWSER_CAPACITY_GROW;

    tmp = (launcher_browser_entry_t *)realloc(
        state->entries,
        sizeof(*state->entries) * (size_t)new_capacity);

    if (!tmp)
        return 0;

    state->entries = tmp;
    state->entry_capacity = new_capacity;
    return 1;
}

int launcher_browser_append_entry(const char *name, const char *full_path, int is_dir)
{
    launcher_browser_state_t *state = launcher_browser_state_mut();
    launcher_browser_entry_t *entry;

    if (!launcher_browser_ensure_capacity(state->entry_count + 1))
        return 0;

    entry = &state->entries[state->entry_count];
    snprintf(entry->name, sizeof(entry->name), "%s", name ? name : "");
    snprintf(entry->full_path, sizeof(entry->full_path), "%s", full_path ? full_path : "");
    entry->is_dir = is_dir ? 1 : 0;
    state->entry_count++;
    return 1;
}

void launcher_browser_init(void)
{
    launcher_browser_close_scan_dir();
    launcher_browser_clear_entries();
    launcher_browser_state_reset();
}

int launcher_browser_last_error(void)
{
    return launcher_browser_state_get()->last_error;
}

void launcher_browser_clear_error(void)
{
    launcher_browser_state_mut()->last_error = 0;
}
