#include "save.h"

#include "game.h"
#include "common/inf_log.h"
#include "common/inf_paths.h"
#include "libretro.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define APP_GAME_SAVE_DIR "srm"
#define APP_GAME_SAVE_EXT ".srm"

static int app_game_is_path_sep(char c)
{
    return c == '/' || c == '\\' || c == ':';
}

static void app_game_build_save_stem(const char *identity_path,
                                     char *out,
                                     size_t out_size)
{
    const char *base;
    const char *p;
    const char *end;
    size_t len = 0;

    if (!out || out_size == 0)
        return;

    out[0] = '\0';

    if (!identity_path || !identity_path[0]) {
        snprintf(out, out_size, "save");
        return;
    }

    base = identity_path;
    for (p = identity_path; *p; p++) {
        if (app_game_is_path_sep(*p))
            base = p + 1;
    }

    end = base + strlen(base);

    for (p = base; *p; p++) {
        if (*p == ';') {
            end = p;
            break;
        }
    }

    for (p = end; p > base; ) {
        p--;
        if (*p == '.') {
            end = p;
            break;
        }
    }

    while (base < end && len + 1 < out_size) {
        char c = *base++;

        if (c == '<' || c == '>' || c == '"' || c == '|' ||
            c == '?' || c == '*' || c == ';')
            c = '_';

        out[len++] = c;
    }

    out[len] = '\0';

    if (!out[0])
        snprintf(out, out_size, "save");
}

static int app_game_build_save_path(char *out, size_t out_size)
{
    char stem[256];
    const char *identity_path;

    if (!out || out_size == 0)
        return 0;

    out[0] = '\0';

    identity_path = app_game_loaded_identity_path();
    if (!identity_path || !identity_path[0])
        return 0;

    app_game_build_save_stem(identity_path, stem, sizeof(stem));
    if (!stem[0])
        return 0;

    if (!INF_SNPRINTF_OK(out, out_size,
                         APP_GAME_SAVE_DIR "/%s" APP_GAME_SAVE_EXT, stem)) {
        out[0] = '\0';
        return 0;
    }

    return 1;
}

static int app_game_ensure_save_dir(void)
{
    if (mkdir(APP_GAME_SAVE_DIR, 0777) == 0)
        return 1;

    if (errno == EEXIST)
        return 1;

    printf("[DBG] app_game_ensure_save_dir: mkdir('%s') failed errno=%d\n",
           APP_GAME_SAVE_DIR, errno);
    fflush(stdout);
    return 0;
}

int app_game_sram_autoload(void)
{
    char save_path[INF_PATH_MAX];
    void *save_data;
    size_t save_size;
    size_t read_size;
    FILE *fp;

    if (!app_game_build_save_path(save_path, sizeof(save_path))) {
        printf("[DBG] app_game_sram_autoload: no identity path\n");
        fflush(stdout);
        return 0;
    }

    save_size = retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
    save_data = retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);

    if (!save_data || save_size == 0) {
        printf("[DBG] app_game_sram_autoload: no SRAM exposed by core\n");
        fflush(stdout);
        return 0;
    }

    fp = fopen(save_path, "rb");
    if (!fp) {
        printf("[DBG] app_game_sram_autoload: no save file '%s'\n", save_path);
        fflush(stdout);
        return 0;
    }

    memset(save_data, 0, save_size);
    read_size = fread(save_data, 1, save_size, fp);
    fclose(fp);

    if (read_size < save_size) {
        memset((unsigned char *)save_data + read_size, 0, save_size - read_size);
    }

    printf("[DBG] app_game_sram_autoload: loaded '%s' (%u bytes)\n",
           save_path,
           (unsigned)read_size);
    fflush(stdout);
    return 1;
}

int app_game_sram_autosave(void)
{
    char save_path[INF_PATH_MAX];
    void *save_data;
    unsigned char *bytes;
    size_t save_size;
    size_t used_size;
    size_t written;
    FILE *fp;

    if (!app_game_build_save_path(save_path, sizeof(save_path))) {
        printf("[DBG] app_game_sram_autosave: no identity path\n");
        fflush(stdout);
        return 0;
    }

    save_size = retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
    save_data = retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);

    if (!save_data || save_size == 0) {
        printf("[DBG] app_game_sram_autosave: no SRAM exposed by core\n");
        fflush(stdout);
        return 0;
    }

    bytes = (unsigned char *)save_data;
    used_size = save_size;
    while (used_size > 0 && bytes[used_size - 1] == 0)
        used_size--;

    if (used_size == 0) {
        if (unlink(save_path) == 0) {
            printf("[DBG] app_game_sram_autosave: deleted empty save '%s'\n", save_path);
            fflush(stdout);
        }
        return 1;
    }

    if (!app_game_ensure_save_dir())
        return 0;

    fp = fopen(save_path, "wb");
    if (!fp) {
        printf("[DBG] app_game_sram_autosave: fopen('%s') failed\n", save_path);
        fflush(stdout);
        return 0;
    }

    written = fwrite(save_data, 1, used_size, fp);
    fclose(fp);

    if (written != used_size) {
        printf("[DBG] app_game_sram_autosave: short write '%s' (%u/%u)\n",
               save_path,
               (unsigned)written,
               (unsigned)used_size);
        fflush(stdout);
        return 0;
    }

    printf("[DBG] app_game_sram_autosave: saved '%s' (%u bytes)\n",
           save_path,
           (unsigned)used_size);
    fflush(stdout);
    return 1;
}
