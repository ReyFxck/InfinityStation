#include <stdio.h>
#include <string.h>

#include "app_game.h"
#include "rom_loader/rom_loader.h"
#include "libretro.h"
#include "ui/launcher/launcher.h"

extern unsigned char smw_sfc_start[];
extern unsigned char smw_sfc_end[];

static void *g_loaded_rom_data = NULL;
static size_t g_loaded_rom_size = 0;
static char g_loaded_rom_name[256];

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
    const char *path = launcher_selected_path();

    memset(&game, 0, sizeof(game));

    printf("[DBG] app_game_load_selected: selected_path='%s'\n",
           (path && path[0]) ? path : "<builtin>");
    fflush(stdout);

    if (path && path[0]) {
        app_game_unload_loaded();

        if (!rom_loader_load(path, &g_loaded_rom_data, &g_loaded_rom_size,
                             g_loaded_rom_name, sizeof(g_loaded_rom_name))) {
            printf("[DBG] rom_loader_load() FALHOU: path='%s'\n", path);
            fflush(stdout);
            return 0;
        }

        game.path = g_loaded_rom_name[0] ? g_loaded_rom_name : path;
        game.data = g_loaded_rom_data;
        game.size = g_loaded_rom_size;
        game.meta = NULL;

        printf("[DBG] rom_loader_load() OK: raw_path='%s'\n", path);
        printf("[DBG] retro_game_info.path='%s'\n", game.path ? game.path : "<null>");
        printf("[DBG] retro_game_info.size=%u\n", (unsigned)game.size);
        fflush(stdout);

        if (retro_load_game(&game)) {
            printf("[DBG] retro_load_game() OK\n");
            fflush(stdout);
            return 1;
        }

        printf("[DBG] retro_load_game() FALHOU: raw_path='%s' final_path='%s' size=%u\n",
               path,
               game.path ? game.path : "<null>",
               (unsigned)game.size);
        fflush(stdout);

        app_game_unload_loaded();
        return 0;
    }

    g_loaded_rom_name[0] = '\0';
    game.path = "smw.sfc";
    game.data = smw_sfc_start;
    game.size = (size_t)(smw_sfc_end - smw_sfc_start);
    game.meta = NULL;

    printf("[DBG] usando ROM embutida: path='%s' size=%u\n",
           game.path, (unsigned)game.size);
    fflush(stdout);

    if (retro_load_game(&game)) {
        printf("[DBG] retro_load_game() OK com ROM embutida\n");
        fflush(stdout);
        return 1;
    }

    printf("[DBG] retro_load_game() FALHOU com ROM embutida\n");
    fflush(stdout);
    return 0;
}
