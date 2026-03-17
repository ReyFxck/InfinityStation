#include <stdio.h>
#include <string.h>

#include "font/browser_font.h"
#include "launcher_pages_internal.h"

extern const unsigned int launcher_logo_width;
extern const unsigned int launcher_logo_height;

static unsigned screen_center_x(unsigned screen_w, unsigned obj_w)
{
    if (obj_w >= screen_w)
        return 0;

    return (screen_w - obj_w) / 2;
}

static void browser_copy_plain(char *out, size_t out_size, const char *src)
{
    if (!out || out_size == 0)
        return;

    if (!src) {
        out[0] = '\0';
        return;
    }

    snprintf(out, out_size, "%s", src);
}

static void browser_copy_ellipsis(char *out, size_t out_size, const char *src, int visible_chars)
{
    size_t len;
    size_t keep;
    size_t i;

    if (!out || out_size == 0)
        return;

    if (!src) {
        out[0] = '\0';
        return;
    }

    len = strlen(src);

    if ((int)len <= visible_chars) {
        browser_copy_plain(out, out_size, src);
        return;
    }

    if (visible_chars <= 3) {
        snprintf(out, out_size, "...");
        return;
    }

    keep = (size_t)(visible_chars - 3);

    for (i = 0; i < keep && i + 1 < out_size; i++)
        out[i] = src[i];

    if (i + 4 <= out_size) {
        out[i++] = '.';
        out[i++] = '.';
        out[i++] = '.';
    }

    out[i] = '\0';
}

static void browser_copy_marquee(char *out, size_t out_size, const char *src, int visible_chars, unsigned tick)
{
    size_t len;
    size_t i;
    unsigned gap = 6;
    unsigned period;
    unsigned offset;

    if (!out || out_size == 0)
        return;

    if (!src) {
        out[0] = '\0';
        return;
    }

    len = strlen(src);

    if ((int)len <= visible_chars) {
        browser_copy_plain(out, out_size, src);
        return;
    }

    period = (unsigned)len + gap;
    offset = (tick / 8u) % period;

    for (i = 0; i < (size_t)visible_chars && i + 1 < out_size; i++) {
        unsigned pos = offset + (unsigned)i;

        if (pos >= period)
            pos -= period;

        out[i] = (pos < len) ? src[pos] : ' ';
    }

    out[i] = '\0';
}

void launcher_pages_draw_browser_page(void)
{
    char path_line[96];
    int count = launcher_browser_count();
    int scroll = launcher_browser_scroll();
    int selected = launcher_browser_selected();
    int row;

    static int s_prev_selected = -9999;
    static unsigned s_marquee_delay_frames = 0;
    static unsigned s_marquee_tick = 0;

    const int content_x = 105;
    const int content_y = 105;

    const unsigned path_w = 11;
    const unsigned path_h = 17;

    const unsigned item_w = 10;
    const unsigned item_h = 16;

    const uint16_t path_color = 0x2411; /* turquesa */
    const uint16_t normal     = 0x39E7; /* cinza escuro */
    const uint16_t select     = 0x7053; /* roxo */

    const int visible_name_chars = 30;
    const unsigned marquee_delay_frames = 90; /* ~1.5s em 60fps */

    launcher_pages_fit_text(path_line, sizeof(path_line), launcher_browser_current_path(), 34);

    launcher_logo_draw(
        screen_center_x(640, launcher_logo_width),
        18
    );

    browser_font_draw_string_color_sized(
        content_x,
        content_y + 0,
        path_line,
        path_color,
        path_w,
        path_h
    );

    if (selected != s_prev_selected) {
        s_prev_selected = selected;
        s_marquee_delay_frames = 0;
        s_marquee_tick = 0;
    } else {
        const launcher_browser_entry_t *sel_entry = launcher_browser_entry(selected);

        if (sel_entry && (int)strlen(sel_entry->name) > visible_name_chars) {
            if (s_marquee_delay_frames < marquee_delay_frames)
                s_marquee_delay_frames++;
            else
                s_marquee_tick++;
        } else {
            s_marquee_delay_frames = 0;
            s_marquee_tick = 0;
        }
    }

    if (launcher_browser_last_error()) {
        browser_font_draw_string_color_sized(
            content_x,
            content_y + 66,
            "OPEN FAILED",
            select,
            12,
            18
        );
    } else if (count <= 0) {
        browser_font_draw_string_color_sized(
            content_x,
            content_y + 66,
            "NO FOLDERS OR ROMS",
            normal,
            11,
            17
        );
    } else {
        for (row = 0; row < LAUNCHER_BROWSER_ROWS; row++) {
            int index = scroll + row;
            const launcher_browser_entry_t *entry;
            char line[128];
            char name_buf[96];
            uint16_t color;

            if (index >= count)
                break;

            entry = launcher_browser_entry(index);
            if (!entry)
                break;

            if (index == selected && (int)strlen(entry->name) > visible_name_chars) {
                if (s_marquee_delay_frames < marquee_delay_frames)
                    browser_copy_ellipsis(name_buf, sizeof(name_buf), entry->name, visible_name_chars);
                else
                    browser_copy_marquee(name_buf, sizeof(name_buf), entry->name, visible_name_chars, s_marquee_tick);
            } else {
                browser_copy_ellipsis(name_buf, sizeof(name_buf), entry->name, visible_name_chars);
            }

            if (entry->is_dir)
                snprintf(line, sizeof(line), "%sDIR %s", index == selected ? "> " : "  ", name_buf);
            else
                snprintf(line, sizeof(line), "%s%s", index == selected ? "> " : "  ", name_buf);

            color = (index == selected) ? select : normal;

            browser_font_draw_string_color_sized(
                content_x,
                content_y + 40 + row * 17,
                line,
                color,
                item_w,
                item_h
            );
        }
    }
}
