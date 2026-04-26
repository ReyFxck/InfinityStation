#include "launcher_browser_internal.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <libmc.h>
#include "rom_loader/rom_loader.h"
#include "disc.h"

#define LAUNCHER_BROWSER_MC_LIST_MAX 256
#define HOST_PROBE_BACKOFF_REFRESHES 15

static int g_mc0_ready = 0;
static int g_mc1_ready = 0;
static int g_mass0_ready = 0;
static int g_mass1_ready = 0;
static int g_disc_ready = 0;
static int g_host_ready = 0;
static int g_host_probe_backoff = 0;
static int g_mc_init_done = 0;
static sceMcTblGetDir g_mc_dir[LAUNCHER_BROWSER_MC_LIST_MAX] __attribute__((aligned(64)));

static int launcher_browser_probe_device(const char *path)
{
    DIR *d = opendir(path);
    if (!d)
        return 0;
    closedir(d);
    return 1;
}

static int launcher_browser_probe_device_any(const char *primary, const char *fallback)
{
    if (primary && launcher_browser_probe_device(primary))
        return 1;
    if (fallback && launcher_browser_probe_device(fallback))
        return 1;
    return 0;
}

static int launcher_browser_mc_port_from_path(const char *path)
{
    if (!path) return -1;
    if (!strncmp(path, "mc0:", 4)) return 0;
    if (!strncmp(path, "mc1:", 4)) return 1;
    return -1;
}

static int launcher_browser_mc_load_module_any(const char *primary, const char *fallback)
{
    int ret;

    ret = SifLoadModule(primary, 0, NULL);
    if (ret >= 0)
        return 1;

    if (!fallback)
        return 0;

    ret = SifLoadModule(fallback, 0, NULL);
    return (ret >= 0) ? 1 : 0;
}

static int launcher_browser_mc_init_once(void)
{
    int ret;

    if (g_mc_init_done)
        return 1;

    sceSifInitRpc(0);

    if (!launcher_browser_mc_load_module_any("rom0:XSIO2MAN", "rom0:SIO2MAN"))
        return 0;
    if (!launcher_browser_mc_load_module_any("rom0:XMCMAN", "rom0:MCMAN"))
        return 0;
    if (!launcher_browser_mc_load_module_any("rom0:XMCSERV", "rom0:MCSERV"))
        return 0;

    ret = mcInit(MC_TYPE_XMC);
#ifdef MC_TYPE_MC
    if (ret < 0)
        ret = mcInit(MC_TYPE_MC);
#endif
    if (ret < 0)
        return 0;

    g_mc_init_done = 1;
    return 1;
}

static int launcher_browser_mc_probe_port(int port)
{
    int type = 0;
    int free_blocks = 0;
    int format = 0;
    int ret = 0;

    if (port < 0 || port > 1)
        return 0;

    if (!launcher_browser_mc_init_once())
        return 0;

    mcGetInfo(port, 0, &type, &free_blocks, &format);
    mcSync(0, NULL, &ret);

    return (ret > -10) ? 1 : 0;
}

static void launcher_browser_mc_pattern_from_path(const char *path, char *out, size_t out_size)
{
    const char *colon;
    const char *rel;

    if (!out || out_size == 0)
        return;

    colon = path ? strchr(path, ':') : NULL;
    rel = colon ? (colon + 1) : NULL;

    if (!rel || !rel[0] || !strcmp(rel, "/")) {
        snprintf(out, out_size, "/*");
        return;
    }

    if (rel[0] == '/')
        snprintf(out, out_size, "%s/*", rel);
    else
        snprintf(out, out_size, "/%s/*", rel);
}

static void launcher_browser_refresh_host_status(void)
{
    if (g_host_ready) {
        g_host_probe_backoff = 0;
        return;
    }

    if (g_host_probe_backoff > 0) {
        g_host_probe_backoff--;
        return;
    }

    g_host_ready = launcher_browser_probe_device("host:");
    if (!g_host_ready)
        g_host_ready = launcher_browser_probe_device("host:/");

    if (!g_host_ready)
        g_host_probe_backoff = HOST_PROBE_BACKOFF_REFRESHES;
}

void launcher_browser_refresh_root_device_statuses(void)
{
    g_mc0_ready = launcher_browser_mc_probe_port(0);
    g_mc1_ready = launcher_browser_mc_probe_port(1);

    g_mass0_ready = launcher_browser_probe_device_any("mass0:", "mass0:/");
    g_mass1_ready = launcher_browser_probe_device_any("mass1:", "mass1:/");

    ps2_disc_init_once();
    g_disc_ready = ps2_disc_refresh();

    launcher_browser_refresh_host_status();
}

int launcher_browser_device_ready(const char *name)
{
    if (!name) return 0;
    if (!strcmp(name, "disc:/")  || !strcmp(name, "disc:"))  return g_disc_ready;
    if (!strcmp(name, "mc0:/")   || !strcmp(name, "mc0:"))   return g_mc0_ready;
    if (!strcmp(name, "mc1:/")   || !strcmp(name, "mc1:"))   return g_mc1_ready;
    if (!strcmp(name, "mass0:/") || !strcmp(name, "mass0:")) return g_mass0_ready;
    if (!strcmp(name, "mass1:/") || !strcmp(name, "mass1:")) return g_mass1_ready;
    if (!strcmp(name, "host:/")  || !strcmp(name, "host:"))  return g_host_ready;
    return 0;
}

int launcher_browser_scan_root_devices(void)
{
    launcher_browser_state_t *state = launcher_browser_state_mut();

    launcher_browser_close_scan_dir();
    launcher_browser_clear_entries();
    g_host_probe_backoff = 0;
    launcher_browser_refresh_root_device_statuses();

    state->current_path[0] = '\0';
    state->selected = 0;
    state->scroll = 0;
    state->last_error = 0;
    state->scan_done = 1;

    if (!launcher_browser_append_entry("disc:/", "disc:/", 1))  return 0;
    if (!launcher_browser_append_entry("mc0:/", "mc0:/", 1))   return 0;
    if (!launcher_browser_append_entry("mc1:/", "mc1:/", 1))   return 0;
    if (!launcher_browser_append_entry("mass0:/", "mass0:/", 1)) return 0;
    if (!launcher_browser_append_entry("mass1:/", "mass1:/", 1)) return 0;
    if (!launcher_browser_append_entry("host:/", "host:/", 1))  return 0;

    return 1;
}

int launcher_browser_scan_memory_card_path(const char *path)
{
    launcher_browser_state_t *state = launcher_browser_state_mut();
    int port;
    int type = 0;
    int free_blocks = 0;
    int format = 0;
    int ret = 0;
    int i;
    char pattern[INF_PATH_MAX];

    port = launcher_browser_mc_port_from_path(path);
    if (port < 0)
        return 0;

    launcher_browser_close_scan_dir();
    launcher_browser_clear_entries();

    snprintf(state->current_path, sizeof(state->current_path), "%s", path ? path : "");
    state->selected = 0;
    state->scroll = 0;
    state->last_error = 0;
    state->scan_done = 1;

    if (!launcher_browser_mc_init_once())
        return 1;

    mcGetInfo(port, 0, &type, &free_blocks, &format);
    mcSync(0, NULL, &ret);
    if (ret <= -10)
        return 1;

    launcher_browser_mc_pattern_from_path(path, pattern, sizeof(pattern));
    mcGetDir(port, 0, pattern, 0, LAUNCHER_BROWSER_MC_LIST_MAX, g_mc_dir);
    mcSync(0, NULL, &ret);
    if (ret < 0)
        return 1;

    for (i = 0; i < ret; i++) {
        const char *name = (const char *)g_mc_dir[i].EntryName;
        int is_dir = (g_mc_dir[i].AttrFile & MC_ATTR_SUBDIR) ? 1 : 0;
        char full[INF_PATH_MAX];

        if (!name[0] || !strcmp(name, ".") || !strcmp(name, ".."))
            continue;

        launcher_browser_path_join(full, sizeof(full), path, name);
        if (!is_dir && !rom_loader_is_supported(full))
            continue;

        if (!launcher_browser_append_entry(name, full, is_dir)) {
            state->last_error = 1;
            return 0;
        }
    }

    launcher_browser_sort_entries();

    if (state->selected >= state->entry_count)
        state->selected = (state->entry_count > 0) ? (state->entry_count - 1) : 0;
    if (state->scroll > state->selected)
        state->scroll = state->selected;
    if (state->scroll < 0)
        state->scroll = 0;

    return 1;
}
