#include "browser_internal.h"
#include "rom_loader/rom_loader.h"
#include "disc.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libcdvd.h>

#define ISO_SECTOR_SIZE 2048

typedef struct iso_dir_info
{
    unsigned int lsn;
    unsigned int size;
    int is_dir;
} iso_dir_info_t;

static int launcher_browser_is_host_path(const char *path)
{
    return path && !strncmp(path, "host:", 5);
}

static int launcher_browser_is_disc_path(const char *path)
{
    return path && !strncmp(path, "disc:", 5);
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

static const char *launcher_browser_disc_rel_base(const char *path)
{
    if (!launcher_browser_is_disc_path(path))
        return NULL;

    path += 5;
    while (*path == '/')
        path++;

    return path;
}


static void launcher_browser_host_join(char *out, size_t out_size,
                                       const char *base,
                                       const char *name)
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

static unsigned int iso_le32(const unsigned char *p)
{
    return ((unsigned int)p[0]) |
           ((unsigned int)p[1] << 8) |
           ((unsigned int)p[2] << 16) |
           ((unsigned int)p[3] << 24);
}

static int iso_name_equals_ci(const char *a, const char *b)
{
    size_t i = 0;

    if (!a || !b)
        return 0;

    while (a[i] && b[i]) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];

        if (ca >= 'a' && ca <= 'z')
            ca = (unsigned char)(ca - ('a' - 'A'));
        if (cb >= 'a' && cb <= 'z')
            cb = (unsigned char)(cb - ('a' - 'A'));

        if (ca != cb)
            return 0;
        i++;
    }

    return a[i] == '\0' && b[i] == '\0';
}

static void iso_extract_name_raw(const unsigned char *rec, char *out, size_t out_size)
{
    unsigned int len = rec[32];
    const unsigned char *src = rec + 33;
    size_t i, j = 0;

    if (!out || out_size == 0)
        return;

    if (len == 1 && (src[0] == 0 || src[0] == 1)) {
        out[0] = '\0';
        return;
    }

    for (i = 0; i < len && j + 1 < out_size; i++)
        out[j++] = (char)src[i];

    out[j] = '\0';
}

static void iso_extract_name(const unsigned char *rec, char *out, size_t out_size)
{
    char raw[128];
    size_t i = 0;

    iso_extract_name_raw(rec, raw, sizeof(raw));

    if (!out || out_size == 0)
        return;

    while (raw[i] && raw[i] != ';' && i + 1 < out_size) {
        out[i] = raw[i];
        i++;
    }
    out[i] = '\0';
}

static int iso_read_sector(unsigned int lsn, unsigned char *buf)
{
    sceCdRMode mode;

    memset(&mode, 0, sizeof(mode));
    mode.trycount = 0;
    mode.spindlctrl = SCECdSpinNom;
    mode.datapattern = SCECdSecS2048;
    mode.pad = 0;

    if (!sceCdRead(lsn, 1, buf, &mode))
        return 0;

    sceCdSync(0);
    return 1;
}

static int iso_get_root(iso_dir_info_t *out)
{
    unsigned char sector[ISO_SECTOR_SIZE] __attribute__((aligned(64)));
    const unsigned char *rec;

    if (!out)
        return 0;

    if (!iso_read_sector(16, sector))
        return 0;

    if (sector[0] != 1 || memcmp(sector + 1, "CD001", 5) != 0)
        return 0;

    rec = sector + 156;
    out->lsn = iso_le32(rec + 2);
    out->size = iso_le32(rec + 10);
    out->is_dir = 1;
    return 1;
}

static int iso_find_child(unsigned int dir_lsn, unsigned int dir_size,
                          const char *target_name,
                          iso_dir_info_t *out)
{
    unsigned int sectors = (dir_size + ISO_SECTOR_SIZE - 1) / ISO_SECTOR_SIZE;
    unsigned int s;

    for (s = 0; s < sectors; s++) {
        unsigned char sector[ISO_SECTOR_SIZE] __attribute__((aligned(64)));
        unsigned int pos = 0;

        if (!iso_read_sector(dir_lsn + s, sector))
            return 0;

        while (pos < ISO_SECTOR_SIZE) {
            unsigned int len = sector[pos];
            const unsigned char *rec;
            char name[128];

            if (len == 0)
                break;
            if (pos + len > ISO_SECTOR_SIZE)
                break;

            rec = sector + pos;
            iso_extract_name(rec, name, sizeof(name));

            if (name[0] && iso_name_equals_ci(name, target_name)) {
                if (out) {
                    out->lsn = iso_le32(rec + 2);
                    out->size = iso_le32(rec + 10);
                    out->is_dir = (rec[25] & 0x02) ? 1 : 0;
                }
                return 1;
            }

            pos += len;
        }
    }

    return 0;
}

static int iso_walk_path(const char *rel_path, iso_dir_info_t *out)
{
    iso_dir_info_t cur;
    const char *p = rel_path;
    char seg[128];

    if (!iso_get_root(&cur))
        return 0;

    if (!p || !p[0]) {
        if (out)
            *out = cur;
        return 1;
    }

    while (*p) {
        size_t j = 0;
        iso_dir_info_t next;

        while (*p == '/')
            p++;
        if (!*p)
            break;

        while (*p && *p != '/' && j + 1 < sizeof(seg))
            seg[j++] = *p++;
        seg[j] = '\0';

        if (!seg[0])
            continue;

        if (!iso_find_child(cur.lsn, cur.size, seg, &next))
            return 0;

        cur = next;
    }

    if (out)
        *out = cur;
    return 1;
}

static void iso_build_disc_dir_path(char *out, size_t out_size,
                                    const char *rel_path,
                                    const char *name)
{
    if (!out || out_size == 0)
        return;

    if (!rel_path || !rel_path[0])
        snprintf(out, out_size, "disc:/%s", name ? name : "");
    else
        snprintf(out, out_size, "disc:/%s/%s", rel_path, name ? name : "");
}

static void iso_build_cdrom_file_path(char *out, size_t out_size,
                                      const char *rel_path,
                                      const char *iso_name_raw)
{
    const char prefix[] = "cdrom0:\\\\";
    size_t pos = 0;
    size_t i;

    if (!out || out_size == 0)
        return;

    memcpy(out + pos, prefix, sizeof(prefix) - 1);
    pos += sizeof(prefix) - 1;

    if (rel_path && rel_path[0]) {
        for (i = 0; rel_path[i] && pos + 1 < out_size; i++) {
            char c = rel_path[i];
            out[pos++] = (c == '/') ? '\\\\' : c;
        }

        if (pos + 1 < out_size)
            out[pos++] = '\\\\';
    }

    if (iso_name_raw) {
        for (i = 0; iso_name_raw[i] && pos + 1 < out_size; i++) {
            char c = iso_name_raw[i];
            out[pos++] = (c == '/') ? '\\\\' : c;
        }
    }

    out[pos] = '\0';
}

static int iso_list_dir_filtered(const char *rel_path)
{
    launcher_browser_state_t *state = launcher_browser_state_mut();
    iso_dir_info_t dir;
    unsigned int sectors, s;

    if (!iso_walk_path(rel_path, &dir))
        return 0;
    if (!dir.is_dir)
        return 0;

    sectors = (dir.size + ISO_SECTOR_SIZE - 1) / ISO_SECTOR_SIZE;

    for (s = 0; s < sectors; s++) {
        unsigned char sector[ISO_SECTOR_SIZE] __attribute__((aligned(64)));
        unsigned int pos = 0;

        if (!iso_read_sector(dir.lsn + s, sector))
            return 0;

        while (pos < ISO_SECTOR_SIZE) {
            unsigned int len = sector[pos];
            const unsigned char *rec;
            char name[128];
            char raw_name[128];
            char full_path[INF_PATH_MAX];
            int is_dir;

            if (len == 0)
                break;
            if (pos + len > ISO_SECTOR_SIZE)
                break;

            rec = sector + pos;
            iso_extract_name_raw(rec, raw_name, sizeof(raw_name));
            iso_extract_name(rec, name, sizeof(name));
            is_dir = (rec[25] & 0x02) ? 1 : 0;

            if (name[0] && (is_dir || rom_loader_is_supported(raw_name) || rom_loader_is_supported(name))) {
                if (is_dir)
                    iso_build_disc_dir_path(full_path, sizeof(full_path), rel_path, name);
                else
                    iso_build_cdrom_file_path(full_path, sizeof(full_path), rel_path, raw_name);

                if (!launcher_browser_append_entry(name, full_path, is_dir)) {
                    state->last_error = 1;
                    return 0;
                }
            }

            pos += len;
        }
    }

    launcher_browser_sort_entries();

    if (state->selected >= state->entry_count)
        state->selected = (state->entry_count > 0) ? (state->entry_count - 1) : 0;
    if (state->scroll > state->selected)
        state->scroll = state->selected;
    if (state->scroll < 0)
        state->scroll = 0;

    state->scan_done = 1;
    state->last_error = 0;
    return 1;
}

int launcher_browser_scan_disc_path(const char *path)
{
    launcher_browser_state_t *state = launcher_browser_state_mut();
    const char *rel = launcher_browser_disc_rel_base(path);

    launcher_browser_close_scan_dir();
    launcher_browser_clear_entries();

    snprintf(state->current_path, sizeof(state->current_path), "%s", path ? path : "");
    state->selected = 0;
    state->scroll = 0;
    state->last_error = 0;
    state->scan_done = 1;

    ps2_disc_init_once();
    if (!ps2_disc_refresh())
        return 1;

    if (!rel || !rel[0]) {
        iso_dir_info_t roms;
        if (iso_walk_path("ROMS", &roms) && roms.is_dir) {
            if (!launcher_browser_append_entry("ROMS", "disc:/ROMS", 1)) {
                state->last_error = 1;
                return 0;
            }
            return 1;
        }

        state->last_error = 1;
        return 0;
    }

    if (iso_list_dir_filtered(rel))
        return 1;

    state->last_error = 1;
    return 0;
}

void launcher_browser_path_join(char *out, size_t out_size,
                                const char *base,
                                const char *name)
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
        char full[INF_PATH_MAX];
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
            snprintf(entry_name_buf, sizeof(entry_name_buf), "%s", launcher_browser_basename(de->d_name));
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

        if (!launcher_browser_append_entry(entry_name, full, is_dir)) {
            state->last_error = 1;
            state->scan_done = 1;
            launcher_browser_close_scan_dir();
            return 0;
        }

        loaded++;
    }

    return 1;
}
