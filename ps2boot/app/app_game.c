#include <stdio.h>
#include <string.h>

#include "app_game.h"
#include "frontend_config.h"
#include "app_core_options.h"
#include "rom_loader/rom_loader.h"
#include "libretro.h"
#include "ui/launcher/launcher.h"

static void *g_loaded_rom_data = NULL;
static size_t g_loaded_rom_size = 0;
static char g_loaded_rom_name[256];

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

    if (app_game_ext_equals(path, ".zip"))
        return 1;

    if (!strncmp(path, "host:", 5))
        return 1;

    return 0;
}

void app_game_unload_loaded(void)
{
    if (g_loaded_rom_data) {
        rom_loader_free(&g_loaded_rom_data, &g_loaded_rom_size);
        g_loaded_rom_data = NULL;
        g_loaded_rom_size = 0;
    }

    g_loaded_rom_name[0] = '\0';
}

int app_game_load_selected(void)
{
    struct retro_game_info game;
    const frontend_config_t *cfg = frontend_config_get();
    const char *path = launcher_selected_path();
    int preload = 0;

    memset(&game, 0, sizeof(game));

    printf("[DBG] app_game_load_selected: selected_path='%s'\n",
           (path && path[0]) ? path : "");
    fflush(stdout);

    if (!(path && path[0])) {
        printf("[DBG] nenhuma ROM selecionada\n");
        fflush(stdout);
        return 0;
    }

    preload = app_game_should_preload(path);
    app_game_unload_loaded();

    if (preload) {
        if (!rom_loader_load(path, &g_loaded_rom_data, &g_loaded_rom_size,
                             g_loaded_rom_name, sizeof(g_loaded_rom_name))) {
            printf("[DBG] rom_loader_load() FALHOU: path='%s'\n", path);
            fflush(stdout);
            return 0;
        }

        game.path = g_loaded_rom_name[0] ? g_loaded_rom_name : path;
        game.data = g_loaded_rom_data;
        game.size = g_loaded_rom_size;
    } else {
        game.path = path;
        game.data = NULL;
        game.size = 0;
    }

    game.meta = NULL;

    printf("[DBG] app_game_load_selected mode=%s raw_path='%s'\n",
           preload ? "preload" : "path",
           path);
    printf("[DBG] retro_game_info.path='%s'\n",
           game.path ? game.path : "");
    printf("[DBG] retro_game_info.size=%u\n", (unsigned)game.size);
    fflush(stdout);

    if (retro_load_game(&game)) {
        app_core_apply_runtime_options(cfg->game_reduce_slowdown,
                                       cfg->game_reduce_flicker);
        printf("[DBG] retro_load_game() OK\n");
        fflush(stdout);
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
