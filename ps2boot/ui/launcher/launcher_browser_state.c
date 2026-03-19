#include "launcher_browser_internal.h"

launcher_browser_entry_t *g_entries = NULL;
int g_entry_count = 0;
int g_entry_capacity = 0;
char g_current_path[256];
int g_selected = 0;
int g_scroll = 0;
int g_last_error = 0;
DIR *g_scan_dir = NULL;
int g_scan_done = 1;

int launcher_browser_is_root_path(const char *path)
{
    return !path || !path[0];
}

void launcher_browser_close_scan_dir(void)
{
    if (g_scan_dir) {
        closedir(g_scan_dir);
        g_scan_dir = NULL;
    }
}

void launcher_browser_clear_entries(void)
{
    free(g_entries);
    g_entries = NULL;
    g_entry_count = 0;
    g_entry_capacity = 0;
    g_selected = 0;
    g_scroll = 0;
    g_last_error = 0;
}

int launcher_browser_ensure_capacity(int need)
{
    launcher_browser_entry_t *tmp;
    int new_capacity;

    if (need <= g_entry_capacity)
        return 1;

    new_capacity = g_entry_capacity;
    if (new_capacity < LAUNCHER_BROWSER_CAPACITY_GROW)
        new_capacity = LAUNCHER_BROWSER_CAPACITY_GROW;

    while (new_capacity < need)
        new_capacity += LAUNCHER_BROWSER_CAPACITY_GROW;

    tmp = (launcher_browser_entry_t *)realloc(g_entries, sizeof(*g_entries) * (size_t)new_capacity);
    if (!tmp)
        return 0;

    g_entries = tmp;
    g_entry_capacity = new_capacity;
    return 1;
}

int launcher_browser_append_entry(const char *name, int is_dir)
{
    launcher_browser_entry_t *entry;

    if (!launcher_browser_ensure_capacity(g_entry_count + 1))
        return 0;

    entry = &g_entries[g_entry_count];
    snprintf(entry->name, sizeof(entry->name), "%s", name ? name : "");
    entry->is_dir = is_dir ? 1 : 0;
    g_entry_count++;
    return 1;
}

void launcher_browser_init(void)
{
    launcher_browser_close_scan_dir();
    launcher_browser_clear_entries();

    g_current_path[0] = '\0';
    g_selected = 0;
    g_scroll = 0;
    g_last_error = 0;
    g_scan_done = 1;
}


int launcher_browser_last_error(void)
{
    return g_last_error;
}

void launcher_browser_clear_error(void)
{
    g_last_error = 0;
}
