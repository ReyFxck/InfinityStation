#include "font/browser_font.h"
#include "launcher_pages_internal.h"

#include <stdio.h>

void launcher_pages_draw_browser_page(void)
{
    char path_line[96];
    int count = launcher_browser_count();
    int scroll = launcher_browser_scroll();
    int selected = launcher_browser_selected();
    int row;

    launcher_pages_fit_text(path_line, sizeof(path_line), launcher_browser_current_path(), 48);

    browser_font_draw_string_color_scaled(
        LAUNCHER_BROWSER_TITLE_X, LAUNCHER_BROWSER_TITLE_Y,
        "USB BROWSER",
        LAUNCHER_COLOR_TEXT,
        4
    );

    browser_font_draw_string_color_scaled(
        LAUNCHER_BROWSER_PATH_X, LAUNCHER_BROWSER_PATH_Y,
        path_line,
        LAUNCHER_COLOR_HIGHLIGHT,
        2
    );

    if (launcher_browser_last_error()) {
        browser_font_draw_string_color_scaled(178, 184, "OPEN FAILED", LAUNCHER_COLOR_HIGHLIGHT, 3);
    } else if (count <= 0) {
        browser_font_draw_string_color_scaled(96, 184, "NO FOLDERS OR ROMS", LAUNCHER_COLOR_HIGHLIGHT, 3);
    } else {
        for (row = 0; row < LAUNCHER_BROWSER_ROWS; row++) {
            int index = scroll + row;
            const launcher_browser_entry_t *entry;
            char line[96];
            char name_buf[72];

            if (index >= count)
                break;

            entry = launcher_browser_entry(index);
            if (!entry)
                break;

            launcher_pages_fit_text(name_buf, sizeof(name_buf), entry->name, 40);

            if (entry->is_dir)
                snprintf(line, sizeof(line), "%s DIR %s", index == selected ? ">" : " ", name_buf);
            else
                snprintf(line, sizeof(line), "%s %s", index == selected ? ">" : " ", name_buf);

            browser_font_draw_string_color_scaled(
                LAUNCHER_BROWSER_LIST_X,
                LAUNCHER_BROWSER_LIST_Y + row * LAUNCHER_BROWSER_STEP_Y,
                line,
                index == selected ? LAUNCHER_COLOR_HIGHLIGHT : LAUNCHER_COLOR_TEXT,
                2
            );
        }
    }

    browser_font_draw_string_color_scaled(24, 390, "UP DOWN MOVE  L1 R1 PAGE", LAUNCHER_COLOR_TEXT, 2);
    browser_font_draw_string_color_scaled(24, 414, "X OPEN OR START  O BACK  SELECT RELOAD", LAUNCHER_COLOR_TEXT, 2);
}
