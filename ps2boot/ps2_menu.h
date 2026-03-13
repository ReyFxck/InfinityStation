#ifndef PS2_MENU_H
#define PS2_MENU_H

#include <stdint.h>

void ps2_menu_init(void);
int ps2_menu_is_open(void);
void ps2_menu_open(void);
void ps2_menu_close(void);
void ps2_menu_handle(uint32_t pressed);
void ps2_menu_draw(void);

#endif
