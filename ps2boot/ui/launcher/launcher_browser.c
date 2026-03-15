#include "launcher_browser.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define LAUNCHER_BROWSER_PAGE_ENTRIES 16
#define LAUNCHER_BROWSER_ROOT "mass:/"

static launcher_browser_entry_t g_entries[LAUNCHER_BROWSER_PAGE_ENTRIES];
static char g_current_path[256];
static int g_entry_count = 0;
static int g_selected = 0;
static int g_scroll = 0;
static int g_last_error = 0;
static int g_page_start = 0;
static int g_has_more = 0;

static int has_rom_ext(const char *name)
{
    const char *dot = strrchr(name, '.');

    if (!dot)
        return 0;

    if (!strcmp(dot, ".smc") || !strcmp(dot, ".SMC"))
        return 1;
    if (!strcmp(dot, ".sfc") || !strcmp(dot, ".SFC"))
        return 1;

    return 0;
}

static void path_join(char *out, size_t out_size, const char *base, const char *name)
{
    size_t len = strlen(base);

    if (len > 0 && base[len - 1] == '/')
        snprintf(out, out_size, "%s%s", base, name);
    else
        snprintf(out, out_size, "%s/%s", base, name);
}

static int load_page(int start_index, int select_last)
{
    DIR *d;
    struct dirent *de;
    int match_index = 0;

    g_entry_count = 0;
    g_selected = 0;
    g_scroll = 0;
    g_last_error = 0;
    g_has_more = 0;

    d = opendir(g_current_path);
    if (!d) {
        g_last_error = 1;
        return 0;
    }

    while ((de = readdir(d)) != NULL) {
        char full[512];
        struct stat st;
        int keep = 0;
        int is_dir = 0;

        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;

        if (has_rom_ext(de->d_name)) {
            keep = 1;
            is_dir = 0;
        } else {
            path_join(full, sizeof(full), g_current_path, de->d_name);
            if (stat(full, &st) == 0 && S_ISDIR(st.st_mode)) {
                keep = 1;
                is_dir = 1;
            }
        }

        if (!keep)
            continue;

        if (match_index < start_index) {
            match_index++;
            continue;
        }

        if (g_entry_count >= LAUNCHER_BROWSER_PAGE_ENTRIES) {
            g_has_more = 1;
            break;
        }

        snprintf(g_entries[g_entry_count].name, sizeof(g_entries[g_entry_count].name), "%s", de->d_name);
        g_entries[g_entry_count].is_dir = is_dir;
        g_entry_count++;
        match_index++;
    }

    closedir(d);

    g_page_start = start_index;

    if (g_entry_count <= 0) {
        g_selected = 0;
        g_scroll = 0;
        return 1;
    }

    if (select_last)
        g_selected = g_entry_count - 1;
    else
        g_selected = 0;

    if (g_selected < g_scroll)
        g_scroll = g_selected;

    return 1;
}

void launcher_browser_init(void)
{
    snprintf(g_current_path, sizeof(g_current_path), "%s", LAUNCHER_BROWSER_ROOT);
    g_entry_count = 0;
    g_selected = 0;
    g_scroll = 0;
    g_last_error = 0;
    g_page_start = 0;
    g_has_more = 0;
}

int launcher_browser_open(const char *path)
{
    if (!path || !path[0])
        return 0;

    snprintf(g_current_path, sizeof(g_current_path), "%s", path);
    g_page_start = 0;
    return load_page(0, 0);
}

int launcher_browser_refresh(void)
{
    return load_page(g_page_start, 0);
}

int launcher_browser_go_parent(void)
{
    char temp[256];
    char *slash;

    if (!strcmp(g_current_path, LAUNCHER_BROWSER_ROOT))
        return 0;

    snprintf(temp, sizeof(temp), "%s", g_current_path);

    if (strlen(temp) > 6 && temp[strlen(temp) - 1] == '/')
        temp[strlen(temp) - 1] = '\0';

    slash = strrchr(temp, '/');
    if (!slash || slash <= temp + 5)
        snprintf(temp, sizeof(temp), "%s", LAUNCHER_BROWSER_ROOT);
    else
        *slash = '\0';

    return launcher_browser_open(temp);
}

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

void launcher_browser_move(int delta, int visible_rows)
{
    int max_scroll;

    if (g_entry_count <= 0)
        return;

    while (delta > 0) {
        if (g_selected + 1 < g_entry_count) {
            g_selected++;
        } else if (g_has_more) {
            if (!load_page(g_page_start + g_entry_count, 0))
                break;
        } else {
            break;
        }
        delta--;
    }

    while (delta < 0) {
        if (g_selected > 0) {
            g_selected--;
        } else if (g_page_start > 0) {
            int prev_start = g_page_start - LAUNCHER_BROWSER_PAGE_ENTRIES;
            if (prev_start < 0)
                prev_start = 0;
            if (!load_page(prev_start, 1))
                break;
        } else {
            break;
        }
        delta++;
    }

    if (g_selected < g_scroll)
        g_scroll = g_selected;

    if (g_selected >= g_scroll + visible_rows)
        g_scroll = g_selected - visible_rows + 1;

    max_scroll = g_entry_count - visible_rows;
    if (max_scroll < 0)
        max_scroll = 0;

    if (g_scroll > max_scroll)
        g_scroll = max_scroll;
    if (g_scroll < 0)
        g_scroll = 0;
}

int launcher_browser_activate(char *selected_path, size_t path_size, char *selected_label, size_t label_size)
{
    const launcher_browser_entry_t *entry;
    char full[512];

    if (g_entry_count <= 0)
        return 0;

    entry = launcher_browser_entry(g_selected);
    if (!entry)
        return 0;

    path_join(full, sizeof(full), g_current_path, entry->name);

    if (entry->is_dir) {
        launcher_browser_open(full);
        return 0;
    }

    if (selected_path && path_size)
        snprintf(selected_path, path_size, "%s", full);

    if (selected_label && label_size)
        snprintf(selected_label, label_size, "%s", entry->name);

    return 1;
}
