#ifndef BROWSER_FONT_H
#define BROWSER_FONT_H

#include <stdint.h>

unsigned browser_font_width(void);
unsigned browser_font_height(void);
void browser_font_draw_char(unsigned x, unsigned y, unsigned char ch, uint16_t color);
void browser_font_draw_string(unsigned x, unsigned y, const char *s, uint16_t color);

/*
 * scale por enquanto eh ignorado de proposito,
 * para facilitar a troca direta no browser atual.
 */
void browser_font_draw_string_color_scaled(unsigned x, unsigned y, const char *s, uint16_t color, unsigned scale);

#endif
