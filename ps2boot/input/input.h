#ifndef PS2_INPUT_H
#define PS2_INPUT_H

#include <stdint.h>

#define PS2_INPUT_MAX_PORTS   2u
#define PS2_INPUT_MAX_SLOTS   4u
#define PS2_INPUT_MAX_PLAYERS 5u

int ps2_input_init_once(void);
void ps2_input_shutdown(void);
void ps2_input_poll(void);

uint32_t ps2_input_buttons(void);
uint32_t ps2_input_buttons_port(unsigned port);

uint32_t ps2_input_raw_buttons(void);
uint32_t ps2_input_raw_buttons_port(unsigned port);

unsigned char ps2_input_left_x(void);
unsigned char ps2_input_left_x_port(unsigned port);

unsigned char ps2_input_left_y(void);
unsigned char ps2_input_left_y_port(unsigned port);

int16_t ps2_input_libretro_state(unsigned port, unsigned id);

#endif
