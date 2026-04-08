#include "select_menu_actions_internal.h"
#include "frontend_config.h"

int select_menu_wrap_index(int v, int count)
{
    if (v < 0)
        return count - 1;
    if (v >= count)
        return 0;
    return v;
}

void select_menu_cycle_frame_limit(int dir)
{
    select_menu_state_t *state = select_menu_state_mut();
    state->frame_limit = select_menu_wrap_index(state->frame_limit + dir, 4);
}

int select_menu_game_options_count(void)
{
    const select_menu_state_t *state = select_menu_state_get();
    return state->show_fps ? 8 : 7;
}

void select_menu_actions_init(void)
{
    select_menu_state_reset();
}

int select_menu_actions_is_open(void)
{
    return select_menu_state_get()->open;
}

void select_menu_actions_open(void)
{
    select_menu_state_t *state = select_menu_state_mut();
    const frontend_config_t *cfg = frontend_config_get();

    state->open = 1;
    state->page = SELECT_MENU_PAGE_MAIN;
    state->pending_action = SELECT_MENU_ACTION_NONE;

    state->show_fps = cfg->show_fps;
    state->fps_rainbow = cfg->fps_rainbow;
    state->frame_limit = cfg->frame_limit;
    state->game_vsync = cfg->game_vsync;
    state->game_reduce_slowdown = cfg->game_reduce_slowdown;
    state->game_reduce_flicker = cfg->game_reduce_flicker;
    state->game_frameskip_mode = cfg->game_frameskip_mode;
    state->game_frameskip_threshold = cfg->game_frameskip_threshold;

    if (state->game_sel >= select_menu_game_options_count())
        state->game_sel = select_menu_game_options_count() - 1;
    if (state->game_sel < 0)
        state->game_sel = 0;
}

void select_menu_actions_close(void)
{
    select_menu_state_mut()->open = 0;
}

const select_menu_state_t *select_menu_actions_state(void)
{
    return select_menu_state_get();
}

int select_menu_actions_show_fps_enabled(void)
{
    return select_menu_state_get()->show_fps;
}

int select_menu_actions_fps_rainbow_enabled(void)
{
    return select_menu_state_get()->fps_rainbow;
}

int select_menu_actions_frame_limit_mode(void)
{
    return select_menu_state_get()->frame_limit;
}

int select_menu_actions_game_vsync_enabled(void)
{
    return select_menu_state_get()->game_vsync;
}

int select_menu_actions_pending_action(void)
{
    return select_menu_state_get()->pending_action;
}

void select_menu_actions_clear_pending_action(void)
{
    select_menu_state_mut()->pending_action = SELECT_MENU_ACTION_NONE;
}

void select_menu_actions_handle(uint32_t pressed)
{
    const select_menu_state_t *state = select_menu_state_get();
    select_menu_state_t *mut = select_menu_state_mut();

    if (!state->open)
        return;

    if (pressed & (PAD_SELECT | PAD_SQUARE)) {
        mut->open = 0;
        mut->page = SELECT_MENU_PAGE_MAIN;
        return;
    }

    if (state->page == SELECT_MENU_PAGE_MAIN) {
        select_menu_actions_handle_main(pressed);
        return;
    }

    if (state->page == SELECT_MENU_PAGE_VIDEO ||
        state->page == SELECT_MENU_PAGE_VIDEO_DISPLAY ||
        state->page == SELECT_MENU_PAGE_VIDEO_ASPECT) {
        select_menu_actions_handle_video(pressed);
        return;
    }

    if (state->page == SELECT_MENU_PAGE_GAME_OPTIONS) {
        select_menu_actions_handle_game(pressed);
        return;
    }
}
