#include <stdio.h>
#include <string.h>
#include "launcher_pages_internal.h"

void launcher_pages_fit_text(char *out, size_t out_size, const char *src, int max_chars)
{
    size_t len;

    if (!out || out_size == 0)
        return;

    if (!src) {
        out[0] = '\0';
        return;
    }

    len = strlen(src);
    if ((int)len <= max_chars) {
        snprintf(out, out_size, "%s", src);
        return;
    }

    if (max_chars < 4) {
        out[0] = '\0';
        return;
    }

    snprintf(out, out_size, "...%s", src + (len - (size_t)(max_chars - 3)));
}

void launcher_pages_draw(const launcher_state_t *state)
{
    if (!state)
        return;

    if (state->page == LAUNCHER_PAGE_MAIN)
        launcher_pages_draw_main_page(state);
    else if (state->page == LAUNCHER_PAGE_BROWSER)
        launcher_pages_draw_browser_page();
    else if (state->page == LAUNCHER_PAGE_OPTIONS)
        launcher_pages_draw_options_page();
    else if (state->page == LAUNCHER_PAGE_CREDITS)
        launcher_pages_draw_credits_page();
    else
        launcher_pages_draw_main_page(state);
}
