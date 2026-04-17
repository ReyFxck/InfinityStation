#include "launcher_browser_internal.h"
#include "rom_loader/rom_loader.h"


static int launcher_browser_is_host_path(const char *path)
{
    return path && !strncmp(path, "host:", 5);
}

static int launcher_browser_is_disc_path(const char *path)
{
    return path && !strncmp(path, "disc:", 5);
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

static const char *launcher_browser_disc_rel_base(const char *base)
{
    if (!base || strncmp(base, "disc:", 5) != 0)
        return "";

    base += 5;
    while (*base == '/')
        base++;

    return base;
}

static void launcher_browser_host_join(char *out, size_t out_size,
                                       const char *base,
                                       const char *entry_name)
{
    const char *rel;

    if (!out || out_size == 0)
        return;

    if (!entry_name || !entry_name[0]) {
        out[0] = '\0';
        return;
    }

    rel = launcher_browser_host_rel_base(base);
    if (!rel[0])
        snprintf(out, out_size, "host:%s", entry_name);
    else
        snprintf(out, out_size, "host:%s/%s", rel, entry_name);
}

static void launcher_browser_disc_build_cdrom_path(char *out, size_t out_size, const char *base, const char *entry_name)
{
    const char *base_rel = launcher_browser_disc_rel_base(base);
    const char prefix[] = "cdrom0:\\";
    const char suffix[] = ";1";
    size_t prefix_len = sizeof(prefix) - 1;
    size_t suffix_len = sizeof(suffix) - 1;
    size_t base_len = 0;
    size_t entry_len = 0;
    size_t needed;
    size_t pos = 0;
    size_t i;

    if (!out || out_size == 0)
        return;

    if (!entry_name || !entry_name[0]) {
        out[0] = '\0';
        return;
    }

    if (base_rel && base_rel[0])
        base_len = strlen(base_rel);

    entry_len = strlen(entry_name);
    needed = prefix_len + base_len + (base_len ? 1 : 0) + entry_len + suffix_len + 1;

    if (needed > out_size) {
        out[0] = '\0';
        return;
    }

    memcpy(out + pos, prefix, prefix_len);
    pos += prefix_len;

    if (base_len) {
        for (i = 0; i < base_len; i++) {
            char c = base_rel[i];
            out[pos++] = (c == '/') ? '\\' : c;
        }
        out[pos++] = '\\';
    }

    for (i = 0; i < entry_len; i++) {
        char c = entry_name[i];
        out[pos++] = (c == '/') ? '\\' : c;
    }

    memcpy(out + pos, suffix, suffix_len);
    pos += suffix_len;
    out[pos] = '\0';
}


static void launcher_browser_disc_join(char *out, size_t out_size,
                                       const char *base,
                                       const char *entry_name)
{
    const char *rel;

    if (!out || out_size == 0)
        return;

    if (!entry_name || !entry_name[0]) {
        out[0] = '\0';
        return;
    }

    if (rom_loader_is_supported(entry_name)) {
        launcher_browser_disc_build_cdrom_path(out, out_size, base, entry_name);
        return;
    }

    rel = launcher_browser_disc_rel_base(base);
    if (!rel[0])
        snprintf(out, out_size, "disc:/%s", entry_name);
    else
        snprintf(out, out_size, "disc:/%s/%s", rel, entry_name);
}

static void launcher_browser_build_full_path(char *out, size_t out_size,
                                             const char *current_path,
                                             const char *entry_name)
{
    if (!out || out_size == 0)
        return;

    if (launcher_browser_is_root_path(current_path)) {
        snprintf(out, out_size, "%s", entry_name ? entry_name : "");
        return;
    }

    if (launcher_browser_is_host_path(current_path)) {
        launcher_browser_host_join(out, out_size, current_path, entry_name);
        return;
    }

    if (launcher_browser_is_disc_path(current_path)) {
        launcher_browser_disc_join(out, out_size, current_path, entry_name);
        return;
    }

    launcher_browser_path_join(out, out_size, current_path, entry_name);
}

void launcher_browser_move(int delta, int visible_rows)
{
    launcher_browser_state_t *state = launcher_browser_state_mut();
    int steps;
    int before;

    if (visible_rows <= 0)
        visible_rows = 1;

    if (state->entry_count <= 0 && state->scan_done)
        return;

    if (delta > 0) {
        for (steps = 0; steps < delta; steps++) {
            if (state->selected + 1 < state->entry_count) {
                state->selected++;
            } else if (!state->scan_done) {
                before = state->entry_count;
                if (!launcher_browser_load_more_entries(LAUNCHER_BROWSER_LOAD_CHUNK))
                    break;
                if (state->entry_count <= before)
                    break;
                if (state->selected + 1 < state->entry_count)
                    state->selected++;
            } else {
                break;
            }
        }
    } else if (delta < 0) {
        for (steps = 0; steps < -delta; steps++) {
            if (state->selected > 0)
                state->selected--;
            else
                break;
        }
    }

    if (state->selected < state->scroll)
        state->scroll = state->selected;
    if (state->selected >= state->scroll + visible_rows)
        state->scroll = state->selected - visible_rows + 1;
    if (state->scroll < 0)
        state->scroll = 0;
}

int launcher_browser_activate(char *selected_path, size_t path_size, char *selected_label, size_t label_size)
{
    launcher_browser_state_t *state = launcher_browser_state_mut();
    const launcher_browser_entry_t *entry;
    char full[INF_PATH_MAX];
    char previous_path[INF_PATH_MAX];

    entry = launcher_browser_entry(state->selected);
    if (!entry)
        return 0;

    snprintf(previous_path, sizeof(previous_path), "%s", state->current_path);

    if (selected_label && label_size > 0)
        snprintf(selected_label, label_size, "%s", entry->name);

    if (entry->full_path[0])
        snprintf(full, sizeof(full), "%s", entry->full_path);
    else
        launcher_browser_build_full_path(full, sizeof(full), state->current_path, entry->name);

    if (!full[0]) {
        state->last_error = 1;
        return 0;
    }

    if (entry->is_dir) {
        if (launcher_browser_open(full)) {
            state->last_error = 0;
            return 0;
        }

        if (previous_path[0])
            launcher_browser_open(previous_path);
        else
            launcher_browser_open(LAUNCHER_BROWSER_ROOT);

        state->last_error = 1;
        return 0;
    }

    if (selected_path && path_size > 0)
        snprintf(selected_path, path_size, "%s", full);

    state->last_error = 0;
    return 1;
}

