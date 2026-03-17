#ifndef BROWSER_FONT_H
#define BROWSER_FONT_H

#include <stdint.h>

unsigned browser_font_width(void);
unsigned browser_font_height(void);
void browser_font_draw_char(unsigned x, unsigned y, unsigned char ch, uint16_t color);
void browser_font_draw_string(unsigned x, unsigned y, const char *s, uint16_t color);
void browser_font_draw_string_color_scaled(unsigned x, unsigned y, const char *s, uint16_t color, unsigned scale);

/* tamanho livre */
void browser_font_draw_char_sized(unsigned x, unsigned y, unsigned char ch, uint16_t color, unsigned out_w, unsigned out_h);
void browser_font_draw_string_color_sized(unsigned x, unsigned y, const char *s, uint16_t color, unsigned out_w, unsigned out_h);

#endif
