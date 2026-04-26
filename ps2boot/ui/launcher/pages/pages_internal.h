#ifndef LAUNCHER_PAGES_INTERNAL_H
#define LAUNCHER_PAGES_INTERNAL_H

#include <stddef.h>
#include "pages.h"
#include "../browser/browser.h"
#include "../font/font.h"
#include "../logo/logo.h"
#include "../theme.h"

void launcher_pages_fit_text(char *out, size_t out_size, const char *src, int max_chars);
void launcher_pages_draw_main_page(const launcher_state_t *state);
void launcher_pages_draw_browser_page(void);
void launcher_pages_draw_options_page(void);
void launcher_pages_draw_credits_page(void);

#endif
