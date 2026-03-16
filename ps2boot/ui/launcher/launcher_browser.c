#include "launcher_browser_internal.h"

int launcher_browser_count(void)
{
    return g_entry_count;
}

int launcher_browser_selected(void)
{
    return g_selected;
}

int launcher_browser_scroll(void)
{
    return g_scroll;
}

const char *launcher_browser_current_path(void)
{
    if (launcher_browser_is_root_path(g_current_path))
        return LAUNCHER_BROWSER_ROOT_LABEL;

    return g_current_path;
}

const launcher_browser_entry_t *launcher_browser_entry(int index)
{
    if (index < 0 || index >= g_entry_count)
        return NULL;

    return &g_entries[index];
}

int launcher_browser_last_error(void)
{
    return g_last_error;
}
