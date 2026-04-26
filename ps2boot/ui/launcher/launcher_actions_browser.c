#include "launcher_actions_internal.h"
#include "input.h"
#include "rom_loader/rom_loader.h"

enum {
    BROWSER_NAV_NONE = 0,
    BROWSER_NAV_SLOW = 1,
    BROWSER_NAV_MEDIUM = 2,
    BROWSER_NAV_FAST = 3
};

static int g_browser_nav_dir = 0;
static int g_browser_nav_speed = 0;
static int g_browser_nav_hold = 0;

static void browser_nav_reset(void)
{
    g_browser_nav_dir = 0;
    g_browser_nav_speed = 0;
    g_browser_nav_hold = 0;
}

static int browser_nav_repeat_start(int speed)
{
    switch (speed) {
        case BROWSER_NAV_SLOW: return 20;
        case BROWSER_NAV_FAST: return 6;
        case BROWSER_NAV_MEDIUM:
        default: return 12;
    }
}

static int browser_nav_repeat_step(int speed)
{
    switch (speed) {
        case BROWSER_NAV_SLOW: return 9;
        case BROWSER_NAV_FAST: return 2;
        case BROWSER_NAV_MEDIUM:
        default: return 4;
    }
}

static int browser_nav_analog_dir_y(int *speed_out)
{
    int delta = (int)ps2_input_left_y() - 0x80;
    int mag = (delta < 0) ? -delta : delta;

    if (speed_out)
        *speed_out = BROWSER_NAV_NONE;

    if (mag < 0x10)
        return 0;

    if (speed_out) {
        if (mag < 0x28)
            *speed_out = BROWSER_NAV_SLOW;
        else if (mag < 0x50)
            *speed_out = BROWSER_NAV_MEDIUM;
        else
            *speed_out = BROWSER_NAV_FAST;
    }

    return (delta < 0) ? -1 : 1;
}

static int browser_nav_current_dir(int *speed_out)
{
    uint32_t raw = ps2_input_raw_buttons();
    int dir;

    if (speed_out)
        *speed_out = BROWSER_NAV_NONE;

    if (raw & PAD_UP) {
        if (speed_out)
            *speed_out = BROWSER_NAV_MEDIUM;
        return -1;
    }

    if (raw & PAD_DOWN) {
        if (speed_out)
            *speed_out = BROWSER_NAV_MEDIUM;
        return 1;
    }

    dir = browser_nav_analog_dir_y(speed_out);
    if (dir != 0)
        return dir;

    return 0;
}

static int browser_nav_take_step(void)
{
    int speed = BROWSER_NAV_NONE;
    int dir = browser_nav_current_dir(&speed);
    int start;
    int step;

    if (dir == 0 || speed == BROWSER_NAV_NONE) {
        browser_nav_reset();
        return 0;
    }

    if (dir != g_browser_nav_dir || speed != g_browser_nav_speed) {
        g_browser_nav_dir = dir;
        g_browser_nav_speed = speed;
        g_browser_nav_hold = 0;
        return dir;
    }

    g_browser_nav_hold++;
    start = browser_nav_repeat_start(speed);
    step = browser_nav_repeat_step(speed);

    if (g_browser_nav_hold < start)
        return 0;
    if (((g_browser_nav_hold - start) % step) == 0)
        return dir;

    return 0;
}

void launcher_actions_handle_browser(uint32_t pressed)
{
    launcher_state_t *state = launcher_state_mut();
    int wants_activate = (pressed & (PAD_START | PAD_CROSS)) != 0;
    int wants_page_up = (pressed & PAD_L1) != 0;
    int wants_page_down = (pressed & PAD_R1) != 0;
    int nav_step = (wants_activate || wants_page_up || wants_page_down)
        ? 0
        : browser_nav_take_step();

    if (nav_step < 0)
        launcher_browser_move(-1, LAUNCHER_BROWSER_ROWS);
    else if (nav_step > 0)
        launcher_browser_move(1, LAUNCHER_BROWSER_ROWS);

    if (wants_page_up)
        launcher_browser_move(-LAUNCHER_BROWSER_ROWS, LAUNCHER_BROWSER_ROWS);
    if (wants_page_down)
        launcher_browser_move(LAUNCHER_BROWSER_ROWS, LAUNCHER_BROWSER_ROWS);

    if (pressed & PAD_SELECT)
        launcher_browser_refresh();

    if (wants_activate) {
        int activate_result;

        browser_nav_reset();
        state->selected_path[0] = '\0';

        activate_result = launcher_browser_activate(
            state->selected_path,
            sizeof(state->selected_path),
            state->selected_label,
            sizeof(state->selected_label)
        );

        if (activate_result > 0 &&
            state->selected_path[0] != '\0' &&
            rom_loader_is_supported(state->selected_path))
            state->should_start_game = 1;
    }

    if (pressed & PAD_CIRCLE) {
        browser_nav_reset();

        if (launcher_browser_last_error()) {
            launcher_browser_clear_error();
            return;
        }

        if (!launcher_browser_go_parent())
            state->page = LAUNCHER_PAGE_MAIN;
    }
}
