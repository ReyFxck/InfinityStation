#ifndef APP_GAME_H
#define APP_GAME_H

int app_game_load_selected(void);
void app_game_unload_loaded(void);
const char *app_game_loaded_identity_path(void);
int app_game_sram_autoload(void);
int app_game_sram_autosave(void);

#endif
