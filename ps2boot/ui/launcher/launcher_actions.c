#include "launcher_actions_internal.h"

#include "ps2_input.h"

static int g_launcher_input_armed = 0;

int launcher_actions_wrap_index(int v, int count)
{
    if (v < 0)
        return count - 1;
    if (v >= count)
        return 0;
    return v;
}

void launcher_actions_init(void)
{
    launcher_state_reset();
    g_launcher_input_armed = 0;
    launcher_browser_init();
    launcher_browser_open("");
}

const launcher_state_t *launcher_actions_state(void)
{
    return launcher_state_get();
}

int launcher_actions_should_start_game(void)
{
    return launcher_state_get()->should_start_game;
}

void launcher_actions_clear_start_request(void)
{
    launcher_state_mut()->should_start_game = 0;
}

const char *launcher_actions_selected_path(void)
{
    return launcher_state_get()->selected_path;
}

const char *launcher_actions_selected_label(void)
{
    return launcher_state_get()->selected_label;
}

void launcher_actions_open_browser_page(void)
{
    launcher_state_mut()->page = LAUNCHER_PAGE_BROWSER;
}

void launcher_actions_handle(uint32_t pressed)
{
    const launcher_state_t *state = launcher_state_get();
    uint32_t held = ps2_input_buttons();
    uint32_t block_mask =
        PAD_UP | PAD_DOWN | PAD_LEFT | PAD_RIGHT |
        PAD_START | PAD_CROSS | PAD_CIRCLE |
        PAD_L1 | PAD_R1 | PAD_SELECT;

    if (!g_launcher_input_armed) {
        if ((held & block_mask) == 0)
            g_launcher_input_armed = 1;
        else
            return;
    }

    if (state->page == LAUNCHER_PAGE_MAIN) {
        launcher_actions_handle_main(pressed);
        return;
    }

    if (state->page == LAUNCHER_PAGE_BROWSER) {
        launcher_actions_handle_browser(pressed);
        return;
    }

    launcher_actions_handle_options(pressed);
}
