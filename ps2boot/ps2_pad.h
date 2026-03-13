#ifndef PS2_PAD_H
#define PS2_PAD_H

#include <stdint.h>
#include <stddef.h>

int ps2_pad_init_once(void);
void ps2_pad_poll(void);
uint32_t ps2_pad_buttons(void);
int ps2_pad_ready(void);
int16_t ps2_pad_libretro_state(unsigned id);

#endif
