#ifndef SELECT_MENU_H
#define SELECT_MENU_H

#include <stdint.h>

void select_menu_init(void);
int select_menu_is_open(void);
void select_menu_open(void);
void select_menu_close(void);
void select_menu_handle(uint32_t pressed);
void select_menu_draw(void);

int select_menu_show_fps_enabled(void);
int select_menu_frame_limit_mode(void);

#endif
