#include "launcher_browser_internal.h"

#include <string.h>

void launcher_browser_move(int delta, int visible_rows)
{
    int steps;
    int before;

    if (visible_rows <= 0)
        visible_rows = 1;

    if (g_entry_count <= 0 && g_scan_done)
        return;

    if (delta > 0) {
        for (steps = 0; steps < delta; steps++) {
            if (g_selected + 1 < g_entry_count) {
                g_selected++;
            } else if (!g_scan_done) {
                before = g_entry_count;
                if (!launcher_browser_load_more_entries(LAUNCHER_BROWSER_LOAD_CHUNK))
                    break;
                if (g_entry_count <= before)
                    break;
                if (g_selected + 1 < g_entry_count)
                    g_selected++;
            } else {
                break;
            }
        }
    } else if (delta < 0) {
        for (steps = 0; steps < -delta; steps++) {
            if (g_selected > 0)
                g_selected--;
            else
                break;
        }
    }

    if (g_selected < g_scroll)
        g_scroll = g_selected;
    if (g_selected >= g_scroll + visible_rows)
        g_scroll = g_selected - visible_rows + 1;
    if (g_scroll < 0)
        g_scroll = 0;
}

int launcher_browser_activate(char *selected_path, size_t path_size, char *selected_label, size_t label_size)
{
    const launcher_browser_entry_t *entry;
    char full[256];
    char previous_path[256];

    entry = launcher_browser_entry(g_selected);
    if (!entry)
        return 0;

    snprintf(previous_path, sizeof(previous_path), "%s", g_current_path);

    if (selected_label && label_size > 0)
        snprintf(selected_label, label_size, "%s", entry->name);

    if (launcher_browser_is_root_path(g_current_path))
        snprintf(full, sizeof(full), "%s", entry->name);
    else
        launcher_browser_path_join(full, sizeof(full), g_current_path, entry->name);

    if (entry->is_dir) {
        if (launcher_browser_open(full))
            return 0;

        if (previous_path[0])
            launcher_browser_open(previous_path);
        else
            launcher_browser_open(LAUNCHER_BROWSER_ROOT);

        g_last_error = 1;
        return 0;
    }

    if (selected_path && path_size > 0)
        snprintf(selected_path, path_size, "%s", full);

    return 1;
}
