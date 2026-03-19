#include "launcher_browser_internal.h"

#include <string.h>

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
    char target[256];
    int failed = 0;

    snprintf(target, sizeof(target), "%s", path ? path : "");

    launcher_browser_close_scan_dir();

    if (launcher_browser_is_root_path(target))
        return launcher_browser_scan_root_devices();

    launcher_browser_clear_entries();
    g_selected = 0;
    g_scroll = 0;
    g_last_error = 0;
    g_scan_done = 1;

    if (!launcher_browser_open_scan_dir(target)) {
        failed = 1;
        launcher_browser_scan_root_devices();
        g_last_error = 1;
        return 0;
    }

    snprintf(g_current_path, sizeof(g_current_path), "%s", target);

    if (!launcher_browser_finish_scan()) {
        launcher_browser_close_scan_dir();
        failed = 1;
        launcher_browser_scan_root_devices();
        g_last_error = 1;
        return 0;
    }

    (void)failed;
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
