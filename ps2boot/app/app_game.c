#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "app_game.h"
#include "frontend_config.h"
#include "app_core_options.h"
#include "rom_loader/rom_loader.h"
#include "common/inf_paths.h"

#include "libretro.h"
#include "ui/launcher/launcher.h"

static void *g_loaded_rom_data = NULL;
static size_t g_loaded_rom_size = 0;
static char g_loaded_rom_name[256];
static char g_loaded_rom_identity_path[INF_PATH_MAX];

#define APP_GAME_SAVE_DIR "srm"
#define APP_GAME_SAVE_EXT ".srm"

static int app_game_ext_equals(const char *path, const char *ext)
{
    const char *dot;
    size_t i;

    if (!path || !ext)
        return 0;

    dot = strrchr(path, '.');
    if (!dot)
        return 0;

    for (i = 0; dot[i] && ext[i]; i++) {
        char a = dot[i];
        char b = ext[i];

        if (a >= 'A' && a <= 'Z')
            a = (char)(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z')
            b = (char)(b - 'A' + 'a');

        if (a != b)
            return 0;
    }

    if (ext[i] != '\0')
        return 0;

    return (dot[i] == '\0' || dot[i] == ';');
}

static int app_game_should_preload(const char *path)
{
    if (!path || !path[0])
        return 0;

#ifdef LOAD_FROM_MEMORY
    return 1;
#else
    if (!strncmp(path, "host:", 5))
        return 1;

    return 0;
#endif
}

static void app_game_build_identity_path(const char *selected_path,
                                         const char *loaded_name,
                                         char *out,
                                         size_t out_size)
{
    const char *a;
    const char *b;
    const char *slash;
    size_t dir_len;

    if (!out || out_size == 0)
        return;

    out[0] = '\0';

    if (!selected_path || !selected_path[0])
        return;

    if (!app_game_ext_equals(selected_path, ".zip") || !loaded_name || !loaded_name[0]) {
        snprintf(out, out_size, "%s", selected_path);
        return;
    }

    a = strrchr(selected_path, '/');
    b = strrchr(selected_path, '\\');
    slash = a;
    if (!slash || (b && b > slash))
        slash = b;

    if (!slash) {
        snprintf(out, out_size, "%s", loaded_name);
        return;
    }

    dir_len = (size_t)(slash - selected_path + 1);
    if (dir_len >= out_size) {
        snprintf(out, out_size, "%s", loaded_name);
        return;
    }

    memcpy(out, selected_path, dir_len);
    out[dir_len] = '\0';
    snprintf(out + dir_len, out_size - dir_len, "%s", loaded_name);
}

const char *app_game_loaded_identity_path(void)
{
    return g_loaded_rom_identity_path[0] ? g_loaded_rom_identity_path : NULL;
}

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

    if (!out || out_size == 0)
        return 0;

    out[0] = '\0';

    if (!g_loaded_rom_identity_path[0])
        return 0;

    app_game_build_save_stem(g_loaded_rom_identity_path, stem, sizeof(stem));
    if (!stem[0])
        return 0;

    snprintf(out, out_size, APP_GAME_SAVE_DIR "/%s" APP_GAME_SAVE_EXT, stem);
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

void app_game_unload_loaded(void)
{
    if (g_loaded_rom_data) {
        rom_loader_free(&g_loaded_rom_data, &g_loaded_rom_size);
        g_loaded_rom_data = NULL;
        g_loaded_rom_size = 0;
    }

    g_loaded_rom_name[0] = '\0';
    g_loaded_rom_identity_path[0] = '\0';
}

int app_game_load_selected(void)
{
    struct retro_game_info game;
    const frontend_config_t *cfg = frontend_config_get();
    const char *path = launcher_selected_path();
    int preload = 0;
    const char *load_mode = "path";

    memset(&game, 0, sizeof(game));

    printf("[DBG] app_game_load_selected: selected_path='%s'\n",
           (path && path[0]) ? path : "");
    fflush(stdout);

    if (!(path && path[0])) {
        printf("[DBG] nenhuma ROM selecionada\n");
        fflush(stdout);
        return 0;
    }

    app_game_unload_loaded();

    preload = app_game_should_preload(path);

    if (preload) {
        if (!rom_loader_load(path,
                             &g_loaded_rom_data,
                             &g_loaded_rom_size,
                             g_loaded_rom_name,
                             sizeof(g_loaded_rom_name))) {
            printf("[DBG] rom_loader_load() FALHOU: path='%s'\n", path);
            fflush(stdout);
            return 0;
        }

        app_game_build_identity_path(path,
                                     g_loaded_rom_name,
                                     g_loaded_rom_identity_path,
                                     sizeof(g_loaded_rom_identity_path));

        game.path = g_loaded_rom_identity_path[0] ? g_loaded_rom_identity_path : path;
        game.data = g_loaded_rom_data;
        game.size = g_loaded_rom_size;
        load_mode = app_game_ext_equals(path, ".zip") ? "zip-preload" : "preload";
    } else {
        snprintf(g_loaded_rom_identity_path,
                 sizeof(g_loaded_rom_identity_path),
                 "%s",
                 path);
        g_loaded_rom_name[0] = '\0';
        game.path = path;
        game.data = NULL;
        game.size = 0;
        load_mode = "path";
    }

    game.meta = NULL;

    printf("[DBG] app_game_load_selected mode=%s raw_path='%s'\n",
           load_mode,
           path);
    printf("[DBG] retro_game_info.path='%s'\n",
           game.path ? game.path : "");
    printf("[DBG] retro_game_info.size=%u\n", (unsigned)game.size);
    fflush(stdout);

    if (retro_load_game(&game)) {
        app_core_apply_runtime_options(cfg->game_reduce_slowdown,
                                       cfg->game_reduce_flicker,
                                       cfg->game_frameskip_mode,
                                       cfg->game_frameskip_threshold);
        app_game_sram_autoload();
        printf("[DBG] retro_load_game() OK\n");
        fflush(stdout);

#ifdef LOAD_FROM_MEMORY
        if (g_loaded_rom_data) {
            rom_loader_free(&g_loaded_rom_data, &g_loaded_rom_size);
            g_loaded_rom_data = NULL;
            g_loaded_rom_size = 0;
            g_loaded_rom_name[0] = '\0';
        }
#endif
        return 1;
    }

    printf("[DBG] retro_load_game() FALHOU: raw_path='%s' final_path='%s' size=%u\n",
           path,
           game.path ? game.path : "",
           (unsigned)game.size);
    fflush(stdout);

    app_game_unload_loaded();
    return 0;
}

