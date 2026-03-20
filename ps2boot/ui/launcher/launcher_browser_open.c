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

static int launcher_browser_is_device_root_path(const char *path)
{
    size_t len;

    if (!path || !path[0])
        return 0;

    len = strlen(path);

    if (len >= 2 && path[len - 2] == ':' && path[len - 1] == '/')
        return 1;

    if (len >= 1 && path[len - 1] == ':')
        return 1;

    return 0;
}

static int launcher_browser_root_index_for_device_path(const char *path)
{
    if (!path || !path[0])
        return -1;

    if (!strncmp(path, "mc0", 3))
        return 0;
    if (!strncmp(path, "mc1", 3))
        return 1;
    if (!strncmp(path, "mass0", 5))
        return 2;
    if (!strncmp(path, "mass1", 5))
        return 3;

    return -1;
}

static int launcher_browser_open_root_with_selection(int index)
{
    if (!launcher_browser_open(LAUNCHER_BROWSER_ROOT))
        return 0;

    if (index >= 0) {
        g_selected = index;
        g_scroll = 0;
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
    int root_index = -1;

    if (launcher_browser_is_root_path(g_current_path))
        return 0;

    if (launcher_browser_is_device_root_path(g_current_path)) {
        root_index = launcher_browser_root_index_for_device_path(g_current_path);
        return launcher_browser_open_root_with_selection(root_index);
    }

    snprintf(temp, sizeof(temp), "%s", g_current_path);

    while ((slash = strrchr(temp, '/')) != NULL) {
        if (slash[1] == '\0') {
            *slash = '\0';
            continue;
        }

        *slash = '\0';
        break;
    }

    if (!temp[0] || launcher_browser_is_device_root_path(temp)) {
        root_index = launcher_browser_root_index_for_device_path(temp[0] ? temp : g_current_path);
        return launcher_browser_open_root_with_selection(root_index);
    }

    return launcher_browser_open(temp);
}
