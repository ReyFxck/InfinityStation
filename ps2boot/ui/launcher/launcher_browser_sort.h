#ifndef LAUNCHER_BROWSER_SORT_H
#define LAUNCHER_BROWSER_SORT_H

#include "launcher_browser.h"

int launcher_browser_name_cmp(const char *a, const char *b);
int launcher_browser_entry_cmp(const launcher_browser_entry_t *a, const launcher_browser_entry_t *b);
void launcher_browser_sort_entries(void);

#endif
