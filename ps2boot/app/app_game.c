#include <stdio.h>
#include <string.h>

#include "app_game.h"
#include "frontend_config.h"
#include "app_core_options.h"
#include "rom_loader/rom_loader.h"
#include "rom_loader/rom_zip.h"
#include "common/inf_paths.h"

#include "libretro.h"
#include "ui/launcher/launcher.h"

static void *g_loaded_rom_data = NULL;
static size_t g_loaded_rom_size = 0;
static char g_loaded_rom_name[256];
static char g_loaded_rom_temp_path[INF_PATH_MAX];

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


static int app_game_try_load_zip_as_temp_path(const char *path,
                                              struct retro_game_info *game)
{
    (void)path;
    (void)game;

#ifdef LOAD_FROM_MEMORY
    return 0;
#else
    if (!path || !game)
        return 0;

    if (!app_game_ext_equals(path, ".zip"))
        return 0;

    if (!rom_zip_extract_to_temp_file(path,
                                      g_loaded_rom_temp_path,
                                      sizeof(g_loaded_rom_temp_path),
                                      g_loaded_rom_name,
                                      sizeof(g_loaded_rom_name))) {
        g_loaded_rom_temp_path[0] = '\0';
        return 0;
    }

    game->path = g_loaded_rom_temp_path;
    game->data = NULL;
    game->size = 0;
    return 1;
#endif
}



void app_game_unload_loaded(void)
{
    if (g_loaded_rom_data) {
        rom_loader_free(&g_loaded_rom_data, &g_loaded_rom_size);
        g_loaded_rom_data = NULL;
        g_loaded_rom_size = 0;
    }

    if (g_loaded_rom_temp_path[0]) {
        remove(g_loaded_rom_temp_path);
        g_loaded_rom_temp_path[0] = '\0';
    }

    g_loaded_rom_name[0] = '\0';
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

    if (app_game_try_load_zip_as_temp_path(path, &game)) {
        load_mode = "zip-temp";
    } else {
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

            game.path = g_loaded_rom_name[0] ? g_loaded_rom_name : path;
            game.data = g_loaded_rom_data;
            game.size = g_loaded_rom_size;
            load_mode = app_game_ext_equals(path, ".zip") ? "zip-preload" : "preload";
        } else {
            game.path = path;
            game.data = NULL;
            game.size = 0;
            load_mode = "path";
        }
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

