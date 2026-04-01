#include <stdio.h>
#include <string.h>
#include "ps2_launcher_video.h"
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
    const unsigned gap = 6;
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
    offset = tick % period;

    for (i = 0; i < (size_t)visible_chars && i + 1 < out_size; i++) {
        unsigned pos = offset + (unsigned)i;
        if (pos >= period)
            pos -= period;
        out[i] = (pos < len) ? src[pos] : ' ';
    }

    out[i] = '\0';
}

static void launcher_pages_draw_status_circle(int x, int y, uint16_t color)
{
    int dx, dy;

    for (dy = -3; dy <= 3; dy++) {
        for (dx = -3; dx <= 3; dx++) {
            int d2 = dx * dx + dy * dy;
            if (d2 <= 9)
                ps2_launcher_video_put_pixel((unsigned)(x + dx), (unsigned)(y + dy), 0x0000);
            if (d2 <= 4)
                ps2_launcher_video_put_pixel((unsigned)(x + dx), (unsigned)(y + dy), color);
        }
    }
}

void launcher_pages_draw_browser_page(void)
{
    char path_line[96];
    int count;
    int scroll;
    int selected;
    int last_error;
    int row;
    const char *current_path;
    static int s_prev_selected = -9999;
    static char s_prev_path[256] = "";
    static unsigned s_marquee_delay_frames = 0;
    static unsigned s_marquee_tick = 0;
    const int content_x = 105;
    const int content_y = 105;
    const unsigned path_w = 11;
    const unsigned path_h = 17;
    const unsigned item_w = 10;
    const unsigned item_h = 16;
    const uint16_t path_color = 0x2411;
    const uint16_t normal = 0x39E7;
    const uint16_t select = 0x7053;
    const int visible_name_chars = 24;
    const unsigned marquee_delay_frames = 4;

    count = launcher_browser_count();
    scroll = launcher_browser_scroll();
    selected = launcher_browser_selected();
    last_error = launcher_browser_last_error();
    current_path = launcher_browser_current_path();

    if (!current_path || !current_path[0])
        current_path = "DEVICES";

    launcher_pages_fit_text(path_line, sizeof(path_line), current_path, 34);

    launcher_logo_draw(screen_center_x(640, launcher_logo_width), 18);
    browser_font_draw_string_color_sized(content_x, content_y + 0, path_line, path_color, path_w, path_h);

    if (selected != s_prev_selected || strcmp(current_path, s_prev_path) != 0) {
        s_prev_selected = selected;
        snprintf(s_prev_path, sizeof(s_prev_path), "%s", current_path);
        s_marquee_delay_frames = 0;
        s_marquee_tick = 0;
    } else {
        const launcher_browser_entry_t *sel_entry = NULL;

        if (selected >= 0 && selected < count)
            sel_entry = launcher_browser_entry(selected);

        if (!last_error && count > 0 && sel_entry && (int)strlen(sel_entry->name) > visible_name_chars) {
            if (s_marquee_delay_frames < marquee_delay_frames)
                s_marquee_delay_frames++;
            else
                s_marquee_tick++;
        } else {
            s_marquee_delay_frames = 0;
            s_marquee_tick = 0;
        }
    }

    if (current_path && !strcmp(current_path, "DEVICES") && !last_error && count > 0) {
        launcher_browser_refresh_root_device_statuses();
        launcher_pages_draw_status_circle(content_x + 15, content_y + 48, launcher_browser_device_ready("mc0:/")   ? 0x07E0 : 0xF800);
        launcher_pages_draw_status_circle(content_x + 15, content_y + 65, launcher_browser_device_ready("mc1:/")   ? 0x07E0 : 0xF800);
        launcher_pages_draw_status_circle(content_x + 15, content_y + 82, launcher_browser_device_ready("mass0:/") ? 0x07E0 : 0xF800);
        launcher_pages_draw_status_circle(content_x + 15, content_y + 99, launcher_browser_device_ready("mass1:/") ? 0x07E0 : 0xF800);
    }

    if (last_error) {
        const char *fail_msg = "OPEN FAILED";
        int fail_w = (int)strlen(fail_msg) * 12;
        int fail_x = 95 + ((450 - fail_w) / 2) - 24;
        int fail_y = content_y + 118;

        browser_font_draw_string_color_sized(fail_x, fail_y, fail_msg, select, 12, 18);
    } else if (count <= 0) {
        const char *empty_msg = "NO FOLDERS OR ROMS";

        if (!strncmp(current_path, "mc0", 3) || !strncmp(current_path, "mc1", 3))
            empty_msg = "EMPTY MEMORY CARD";
        else if (!strncmp(current_path, "mass0", 5) || !strncmp(current_path, "mass1", 5))
            empty_msg = "EMPTY DEVICE";

        browser_font_draw_string_color_sized(content_x, content_y + 66, empty_msg, normal, 11, 17);
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
                continue;

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
