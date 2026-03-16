#ifndef PS2_INPUT_H
#define PS2_INPUT_H

#include <stdint.h>

int ps2_input_init_once(void);
void ps2_input_poll(void);
uint32_t ps2_input_buttons(void);
int16_t ps2_input_libretro_state(unsigned id);

#endif
