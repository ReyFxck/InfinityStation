#include "launcher_browser_internal.h"
#include "rom_loader/rom_loader.h"

static int launcher_browser_is_host_path(const char *path)
{
    return path && !strncmp(path, "host:", 5);
}

static const char *launcher_browser_basename(const char *path)
{
    const char *slash;

    if (!path)
        return "";

    slash = strrchr(path, '/');
    return slash ? (slash + 1) : path;
}

static const char *launcher_browser_host_rel_base(const char *base)
{
    if (!base || strncmp(base, "host:", 5) != 0)
        return "";

    base += 5;
    while (*base == '/')
        base++;

    return base;
}

static void launcher_browser_host_join(char *out, size_t out_size, const char *base, const char *name)
{
    const char *rel;

    if (!out || out_size == 0)
        return;

    if (!name || !name[0]) {
        out[0] = '\0';
        return;
    }

    rel = launcher_browser_host_rel_base(base);

    if (!rel[0])
        snprintf(out, out_size, "host:%s", name);
    else
        snprintf(out, out_size, "host:%s/%s", rel, name);
}

void launcher_browser_path_join(char *out, size_t out_size, const char *base, const char *name)
{
    size_t len;

    if (!out || out_size == 0)
        return;

    if (!base || !base[0]) {
        snprintf(out, out_size, "%s", name ? name : "");
        return;
    }

    len = strlen(base);
    if (len > 0 && (base[len - 1] == '/' || base[len - 1] == ':'))
        snprintf(out, out_size, "%s%s", base, name ? name : "");
    else
        snprintf(out, out_size, "%s/%s", base, name ? name : "");
}

int launcher_browser_open_scan_dir(const char *path)
{
    launcher_browser_state_t *state = launcher_browser_state_mut();

    launcher_browser_close_scan_dir();
    state->scan_dir = opendir(path);

    if (!state->scan_dir) {
        state->last_error = 1;
        state->scan_done = 1;
        return 0;
    }

    state->scan_done = 0;
    state->last_error = 0;
    return 1;
}

int launcher_browser_load_more_entries(int want)
{
    launcher_browser_state_t *state = launcher_browser_state_mut();
    int loaded = 0;

    if (state->scan_done)
        return 1;

    while (loaded < want) {
        struct dirent *de;
        char full[512];
        char entry_name_buf[256];
        const char *entry_name;
        struct stat st;
        int is_dir = 0;
        int is_host;
        int supported;

        de = readdir(state->scan_dir);
        if (!de) {
            state->scan_done = 1;
            launcher_browser_close_scan_dir();
            launcher_browser_sort_entries();

            if (state->selected >= state->entry_count)
                state->selected = (state->entry_count > 0) ? (state->entry_count - 1) : 0;
            if (state->scroll > state->selected)
                state->scroll = state->selected;
            if (state->scroll < 0)
                state->scroll = 0;

            return 1;
        }

        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;

        is_host = launcher_browser_is_host_path(state->current_path);
        entry_name = de->d_name;

        if (is_host) {
            snprintf(entry_name_buf, sizeof(entry_name_buf), "%s",
                     launcher_browser_basename(de->d_name));
            entry_name = entry_name_buf;
        }

        if (!entry_name[0])
            continue;

        if (is_host)
            launcher_browser_host_join(full, sizeof(full), state->current_path, entry_name);
        else
            launcher_browser_path_join(full, sizeof(full), state->current_path, entry_name);

        supported = rom_loader_is_supported(entry_name) || rom_loader_is_supported(full);

        if (is_host) {
            if (supported) {
                is_dir = 0;
            } else if (!strchr(entry_name, '.')) {
                is_dir = 1;
            } else {
                continue;
            }
        } else {
            if (stat(full, &st) == 0)
                is_dir = S_ISDIR(st.st_mode) ? 1 : 0;

            if (!is_dir && !supported)
                continue;
        }

        if (!launcher_browser_append_entry(entry_name, is_dir)) {
            state->last_error = 1;
            state->scan_done = 1;
            launcher_browser_close_scan_dir();
            return 0;
        }

        loaded++;
    }

    return 1;
}
