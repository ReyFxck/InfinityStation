#include "launcher_browser_internal.h"

static int launcher_browser_finish_scan(void)
{
    while (!g_scan_done) {
        int before = g_entry_count;

        if (!launcher_browser_load_more_entries(LAUNCHER_BROWSER_LOAD_CHUNK))
            return 0;

        if (!g_scan_done && g_entry_count == before)
            break;
    }

    return 1;
}

int launcher_browser_reset_to_path(const char *path)
{
    snprintf(g_current_path, sizeof(g_current_path), "%s", path ? path : "");
    launcher_browser_clear_entries();

    if (launcher_browser_is_root_path(g_current_path))
        return launcher_browser_scan_root_devices();

    if (!launcher_browser_open_scan_dir(g_current_path))
        return 0;

    if (!launcher_browser_finish_scan())
        return 0;

    return 1;
}

int launcher_browser_open(const char *path)
{
    if (!path || !path[0])
        return launcher_browser_reset_to_path(LAUNCHER_BROWSER_ROOT);

    return launcher_browser_reset_to_path(path);
}

int launcher_browser_refresh(void)
{
    return launcher_browser_reset_to_path(g_current_path);
}

int launcher_browser_go_parent(void)
{
    char temp[256];
    char *slash;

    if (launcher_browser_is_root_path(g_current_path))
        return 0;

    snprintf(temp, sizeof(temp), "%s", g_current_path);

    while ((slash = strrchr(temp, '/')) != NULL) {
        if (slash[1] == '\0') {
            *slash = '\0';
            continue;
        }

        *slash = '\0';
        break;
    }

    if (!temp[0])
        return launcher_browser_open(LAUNCHER_BROWSER_ROOT);

    return launcher_browser_open(temp);
}
