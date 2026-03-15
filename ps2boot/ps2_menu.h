#ifndef PS2_MENU_H
#define PS2_MENU_H

#include <stdint.h>

enum
{
    PS2_FRAME_LIMIT_AUTO = 0,
    PS2_FRAME_LIMIT_50   = 1,
    PS2_FRAME_LIMIT_60   = 2,
    PS2_FRAME_LIMIT_OFF  = 3
};

void ps2_menu_init(void);
int ps2_menu_is_open(void);
void ps2_menu_open(void);
void ps2_menu_close(void);
void ps2_menu_handle(uint32_t pressed);
void ps2_menu_draw(void);

int ps2_menu_show_fps_enabled(void);
int ps2_menu_fps_rainbow_enabled(void);
int ps2_menu_frame_limit_mode(void);

int ps2_menu_restart_game_requested(void);
void ps2_menu_clear_restart_game_request(void);

int ps2_menu_exit_game_requested(void);
void ps2_menu_clear_exit_game_request(void);

#endif
