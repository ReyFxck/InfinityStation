#include <debug.h>
#include "app_game.h"

#include "rom_loader/rom_loader.h"

#include <stdio.h>
#include <string.h>

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

    if (path && path[0]) {
        app_game_unload_loaded();

        if (!rom_loader_load(path,
                             &g_loaded_rom_data,
                             &g_loaded_rom_size,
                             g_loaded_rom_name,
                             sizeof(g_loaded_rom_name)))
            return 0;

        game.path = g_loaded_rom_name[0] ? g_loaded_rom_name : path;
        game.data = g_loaded_rom_data;
        game.size = g_loaded_rom_size;
        game.meta = NULL;

        scr_printf("rom externa: %s\n", path);
        scr_printf("rom size: %u bytes\n", (unsigned)g_loaded_rom_size);

        if (retro_load_game(&game))
            return 1;

        app_game_unload_loaded();
        return 0;
    }

    g_loaded_rom_name[0] = '\0';
    game.path = "smw.sfc";
    game.data = smw_sfc_start;
    game.size = (size_t)(smw_sfc_end - smw_sfc_start);
    game.meta = NULL;

    scr_printf("embedded rom size: %u bytes\n", (unsigned)game.size);
    return retro_load_game(&game) ? 1 : 0;
}
