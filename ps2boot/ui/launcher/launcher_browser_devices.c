#include "launcher_browser_internal.h"

#include <string.h>
#include <dirent.h>

static int g_mc0_ready = 0;
static int g_mc1_ready = 0;
static int g_mass0_ready = 0;
static int g_mass1_ready = 0;

static int launcher_browser_probe_device(const char *path)
{
    DIR *d = opendir(path);
    if (!d)
        return 0;

    closedir(d);
    return 1;
}

void launcher_browser_refresh_root_device_statuses(void)
{
    g_mc0_ready = launcher_browser_probe_device("mc0:/");
    g_mc1_ready = launcher_browser_probe_device("mc1:/");
    g_mass0_ready = launcher_browser_probe_device("mass0:/");
    g_mass1_ready = launcher_browser_probe_device("mass1:/");
}

int launcher_browser_device_ready(const char *name)
{
    if (!name)
        return 0;

    if (!strcmp(name, "mc0:/") || !strcmp(name, "mc0:"))
        return g_mc0_ready;
    if (!strcmp(name, "mc1:/") || !strcmp(name, "mc1:"))
        return g_mc1_ready;
    if (!strcmp(name, "mass0:/") || !strcmp(name, "mass0:"))
        return g_mass0_ready;
    if (!strcmp(name, "mass1:/") || !strcmp(name, "mass1:"))
        return g_mass1_ready;

    return 0;
}

int launcher_browser_scan_root_devices(void)
{
    launcher_browser_close_scan_dir();
    launcher_browser_clear_entries();

    g_current_path[0] = '\0';
    g_selected = 0;
    g_scroll = 0;
    g_last_error = 0;
    g_scan_done = 1;

    launcher_browser_refresh_root_device_statuses();

    if (!launcher_browser_append_entry("mc0:/", 1))
        return 0;
    if (!launcher_browser_append_entry("mc1:/", 1))
        return 0;
    if (!launcher_browser_append_entry("mass0:/", 1))
        return 0;
    if (!launcher_browser_append_entry("mass1:/", 1))
        return 0;

    return 1;
}
