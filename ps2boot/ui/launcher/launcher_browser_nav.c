#include "launcher_browser_internal.h"

static int launcher_browser_is_soft_root_device(const char *path)
{
    if (!path)
        return 0;

    return !strcmp(path, "mc0:/")   || !strcmp(path, "mc0:")   ||
           !strcmp(path, "mc1:/")   || !strcmp(path, "mc1:")   ||
           !strcmp(path, "mass0:/") || !strcmp(path, "mass0:") ||
           !strcmp(path, "mass1:/") || !strcmp(path, "mass1:");
}

void launcher_browser_move(int delta, int visible_rows)
{
    launcher_browser_state_t *state = launcher_browser_state_mut();
    int steps;
    int before;

    if (visible_rows <= 0)
        visible_rows = 1;

    if (state->entry_count <= 0 && state->scan_done)
        return;

    if (delta > 0) {
        for (steps = 0; steps < delta; steps++) {
            if (state->selected + 1 < state->entry_count) {
                state->selected++;
            } else if (!state->scan_done) {
                before = state->entry_count;
                if (!launcher_browser_load_more_entries(LAUNCHER_BROWSER_LOAD_CHUNK))
                    break;
                if (state->entry_count <= before)
                    break;
                if (state->selected + 1 < state->entry_count)
                    state->selected++;
            } else {
                break;
            }
        }
    } else if (delta < 0) {
        for (steps = 0; steps < -delta; steps++) {
            if (state->selected > 0)
                state->selected--;
            else
                break;
        }
    }

    if (state->selected < state->scroll)
        state->scroll = state->selected;
    if (state->selected >= state->scroll + visible_rows)
        state->scroll = state->selected - visible_rows + 1;
    if (state->scroll < 0)
        state->scroll = 0;
}

int launcher_browser_activate(char *selected_path, size_t path_size,
                              char *selected_label, size_t label_size)
{
    launcher_browser_state_t *state = launcher_browser_state_mut();
    const launcher_browser_entry_t *entry;
    char full[256];
    char previous_path[256];

    entry = launcher_browser_entry(state->selected);
    if (!entry)
        return 0;

    snprintf(previous_path, sizeof(previous_path), "%s", state->current_path);

    if (selected_label && label_size > 0)
        snprintf(selected_label, label_size, "%s", entry->name);

    if (launcher_browser_is_root_path(state->current_path))
        snprintf(full, sizeof(full), "%s", entry->name);
    else
        launcher_browser_path_join(full, sizeof(full), state->current_path, entry->name);

    if (entry->is_dir) {
        if (launcher_browser_open(full))
            return 0;

        if (launcher_browser_is_soft_root_device(full)) {
            state->selected = 0;
            state->scroll = 0;
            state->scan_done = 1;
            state->last_error = 0;
            return 0;
        }

        if (previous_path[0])
            launcher_browser_open(previous_path);
        else
            launcher_browser_open(LAUNCHER_BROWSER_ROOT);

        state->last_error = 1;
        return 0;
    }

    if (selected_path && path_size > 0)
        snprintf(selected_path, path_size, "%s", full);

    return 1;
}
