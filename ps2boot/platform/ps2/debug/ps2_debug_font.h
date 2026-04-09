#ifndef PS2_DEBUG_FONT_H
#define PS2_DEBUG_FONT_H

#include <stdint.h>

void dbg_set_target(uint16_t *buffer, unsigned stride, unsigned width, unsigned height);
void dbg_reset_target(void);
void dbg_draw_string_color(unsigned x, unsigned y, const char *s, uint16_t color);

#endif
