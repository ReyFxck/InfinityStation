#include "launcher_browser_internal.h"

static char launcher_browser_to_lower_ascii(char c)
{
    if (c >= 'A' && c <= 'Z')
        return (char)(c - 'A' + 'a');
    return c;
}

static int launcher_browser_ext_eq(const char *dot, const char *ext)
{
    size_t i = 0;

    if (!dot || !ext)
        return 0;

    while (dot[i] && ext[i]) {
        if (launcher_browser_to_lower_ascii(dot[i]) != launcher_browser_to_lower_ascii(ext[i]))
            return 0;
        i++;
    }

    return dot[i] == '\0' && ext[i] == '\0';
}

static int launcher_browser_has_rom_ext(const char *name)
{
    const char *dot = strrchr(name, '.');
    if (!dot)
        return 0;

    if (launcher_browser_ext_eq(dot, ".smc")) return 1;
    if (launcher_browser_ext_eq(dot, ".sfc")) return 1;
    if (launcher_browser_ext_eq(dot, ".swc")) return 1;
    if (launcher_browser_ext_eq(dot, ".fig")) return 1;
    if (launcher_browser_ext_eq(dot, ".zip")) return 1;

    return 0;
}

int launcher_browser_open_scan_dir(const char *path)
{
    launcher_browser_close_scan_dir();

    g_scan_dir = opendir(path);
    if (!g_scan_dir) {
        g_last_error = 1;
        g_scan_done = 1;
        return 0;
    }

    g_scan_done = 0;
    return 1;
}

void launcher_browser_path_join(char *out, size_t out_size, const char *base, const char *name)
{
    size_t len = strlen(base);

    if (len > 0 && base[len - 1] == '/')
        snprintf(out, out_size, "%s%s", base, name);
    else
        snprintf(out, out_size, "%s/%s", base, name);
}

static int launcher_browser_dirent_is_dir(const struct dirent *de, const char *full)
{
#if defined(DT_DIR)
    if (de->d_type == DT_DIR)
        return 1;
#endif

#if defined(DT_REG)
    if (de->d_type == DT_REG)
        return 0;
#endif

#if defined(DT_LNK)
    if (de->d_type == DT_LNK)
        return 0;
#endif

#if defined(DT_UNKNOWN)
    if (de->d_type != DT_UNKNOWN)
        return 0;
#endif

    {
        struct stat st;
        if (stat(full, &st) == 0 && S_ISDIR(st.st_mode))
            return 1;
    }

    return 0;
}

int launcher_browser_load_more_entries(int want)
{
    int added = 0;

    if (want <= 0)
        want = 1;

    if (g_scan_done)
        return 1;

    while (added < want) {
        struct dirent *de;
        char full[512];
        int is_dir;

        de = readdir(g_scan_dir);
        if (!de) {
            launcher_browser_close_scan_dir();
            g_scan_done = 1;
            break;
        }

        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;

        launcher_browser_path_join(full, sizeof(full), g_current_path, de->d_name);

        is_dir = launcher_browser_dirent_is_dir(de, full);

        if (!is_dir && !launcher_browser_has_rom_ext(de->d_name))
            continue;

        if (!launcher_browser_append_entry(de->d_name, is_dir))
            return 0;

        added++;
    }

    if (g_scan_done)
        launcher_browser_sort_entries();

    return 1;
}
