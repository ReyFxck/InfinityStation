#include <stdio.h>
#include <string.h>

#include "game.h"
#include "frontend_config.h"
#include "core_options.h"
#include "rom_loader/rom_loader.h"
#include "common/inf_paths.h"

#include "libretro.h"
#include "ui/launcher/launcher.h"
#include "common/inf_log.h"

static void *g_loaded_rom_data = NULL;
static size_t g_loaded_rom_size = 0;
static char g_loaded_rom_name[256];
static char g_loaded_rom_identity_path[INF_PATH_MAX];


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

    INF_LOG_DBG("app_game_load_selected: selected_path='%s'\n",
           (path && path[0]) ? path : "");

    if (!(path && path[0])) {
        INF_LOG_DBG("nenhuma ROM selecionada\n");
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
            INF_LOG_DBG("rom_loader_load() FALHOU: path='%s'\n", path);
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

    INF_LOG_DBG("app_game_load_selected mode=%s raw_path='%s'\n",
           load_mode,
           path);
    INF_LOG_DBG("retro_game_info.path='%s'\n",
           game.path ? game.path : "");
    INF_LOG_DBG("retro_game_info.size=%u\n", (unsigned)game.size);

    if (retro_load_game(&game)) {
        app_core_apply_runtime_options(cfg->game_reduce_slowdown,
                                       cfg->game_reduce_flicker,
                                       cfg->game_frameskip_mode,
                                       cfg->game_frameskip_threshold);
        app_game_sram_autoload();
        INF_LOG_DBG("retro_load_game() OK\n");

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

    INF_LOG_DBG("retro_load_game() FALHOU: raw_path='%s' final_path='%s' size=%u\n",
           path,
           game.path ? game.path : "",
           (unsigned)game.size);

    app_game_unload_loaded();
    return 0;
}

