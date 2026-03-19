#include "launcher_browser_internal.h"

int launcher_browser_scan_root_devices(void)
{
    launcher_browser_close_scan_dir();
    launcher_browser_clear_entries();

    g_current_path[0] = '\0';
    g_selected = 0;
    g_scroll = 0;
    g_last_error = 0;
    g_scan_done = 1;

    if (!launcher_browser_append_entry("mass0:/", 1))
        return 0;
    if (!launcher_browser_append_entry("mass1:/", 1))
        return 0;

    return 1;
}
