#include "launcher_browser_internal.h"
#include "rom_loader/rom_loader.h"

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
        struct stat st;
        int is_dir = 0;

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

        launcher_browser_path_join(full, sizeof(full), state->current_path, de->d_name);

        if (stat(full, &st) == 0)
            is_dir = S_ISDIR(st.st_mode) ? 1 : 0;

        if (!is_dir && !rom_loader_is_supported(full))
            continue;

        if (!launcher_browser_append_entry(de->d_name, is_dir)) {
            state->last_error = 1;
            state->scan_done = 1;
            launcher_browser_close_scan_dir();
            return 0;
        }

        loaded++;
    }

    return 1;
}
