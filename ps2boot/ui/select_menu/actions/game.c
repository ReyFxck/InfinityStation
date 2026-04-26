#include "actions_internal.h"
#include "frontend_config.h"
#include "app_core_options.h"

static const frontend_config_t *cfg_get(void)
{
    return frontend_config_get();
}

static void apply_runtime_gameplay_options(void)
{
    const frontend_config_t *cfg = cfg_get();
    app_core_apply_runtime_options(cfg->game_reduce_slowdown,
                                   cfg->game_reduce_flicker,
                                   cfg->game_frameskip_mode,
                                   cfg->game_frameskip_threshold);
}

static void cycle_reduce_slowdown(select_menu_state_t *state, int dir)
{
    state->game_reduce_slowdown = select_menu_wrap_index(
        state->game_reduce_slowdown + dir, 3);
    frontend_config_set_game_reduce_slowdown(state->game_reduce_slowdown);
    apply_runtime_gameplay_options();
}

static void toggle_reduce_flicker(select_menu_state_t *state)
{
    state->game_reduce_flicker = !state->game_reduce_flicker;
    frontend_config_set_game_reduce_flicker(state->game_reduce_flicker);
    apply_runtime_gameplay_options();
}

static void cycle_frameskip_mode(select_menu_state_t *state, int dir)
{
    state->game_frameskip_mode = select_menu_wrap_index(
        state->game_frameskip_mode + dir, 3);
    frontend_config_set_game_frameskip_mode(state->game_frameskip_mode);
    apply_runtime_gameplay_options();
}

static void cycle_frameskip_threshold(select_menu_state_t *state, int dir)
{
    int value = state->game_frameskip_threshold;

    value += (dir < 0) ? -3 : 3;
    if (value < 15)
        value = 60;
    else if (value > 60)
        value = 15;

    state->game_frameskip_threshold = value;
    frontend_config_set_game_frameskip_threshold(value);
    apply_runtime_gameplay_options();
}

void select_menu_actions_handle_game(uint32_t pressed)
{
    select_menu_state_t *state = select_menu_state_mut();
    int count = select_menu_game_options_count();
    int rainbow_index = 1;
    int vsync_index = state->show_fps ? 2 : 1;
    int frame_index = state->show_fps ? 3 : 2;
    int slowdown_index = state->show_fps ? 4 : 3;
    int flicker_index = state->show_fps ? 5 : 4;
    int frameskip_index = state->show_fps ? 6 : 5;
    int threshold_index = state->show_fps ? 7 : 6;
    int activate = (pressed & PAD_LEFT) || (pressed & PAD_RIGHT) ||
                   (pressed & PAD_START) || (pressed & PAD_CROSS);

    if (pressed & PAD_UP)
        state->game_sel = select_menu_wrap_index(state->game_sel - 1, count);
    if (pressed & PAD_DOWN)
        state->game_sel = select_menu_wrap_index(state->game_sel + 1, count);

    if (state->game_sel == 0) {
        if (activate) {
            state->show_fps = !state->show_fps;
            state->fps_rainbow = state->show_fps ? state->fps_rainbow : 0;
            frontend_config_set_show_fps(state->show_fps);
            frontend_config_set_fps_rainbow(state->fps_rainbow);
            if (state->game_sel >= select_menu_game_options_count())
                state->game_sel = select_menu_game_options_count() - 1;
        }
    } else if (state->show_fps && state->game_sel == rainbow_index) {
        if (activate) {
            state->fps_rainbow = !state->fps_rainbow;
            frontend_config_set_fps_rainbow(state->fps_rainbow);
        }
    } else if (state->game_sel == vsync_index) {
        if (activate) {
            state->game_vsync = !state->game_vsync;
            frontend_config_set_game_vsync(state->game_vsync);
        }
    } else if (state->game_sel == frame_index) {
        if (pressed & PAD_LEFT)
            select_menu_cycle_frame_limit(-1);
        if (pressed & PAD_RIGHT)
            select_menu_cycle_frame_limit(1);
        if ((pressed & PAD_START) || (pressed & PAD_CROSS))
            select_menu_cycle_frame_limit(1);
        frontend_config_set_frame_limit(state->frame_limit);
    } else if (state->game_sel == slowdown_index) {
        if (pressed & PAD_LEFT)
            cycle_reduce_slowdown(state, -1);
        if (pressed & PAD_RIGHT)
            cycle_reduce_slowdown(state, 1);
        if ((pressed & PAD_START) || (pressed & PAD_CROSS))
            cycle_reduce_slowdown(state, 1);
    } else if (state->game_sel == flicker_index) {
        if (activate)
            toggle_reduce_flicker(state);
    } else if (state->game_sel == frameskip_index) {
        if (pressed & PAD_LEFT)
            cycle_frameskip_mode(state, -1);
        if (pressed & PAD_RIGHT)
            cycle_frameskip_mode(state, 1);
        if ((pressed & PAD_START) || (pressed & PAD_CROSS))
            cycle_frameskip_mode(state, 1);
    } else if (state->game_sel == threshold_index) {
        if (pressed & PAD_LEFT)
            cycle_frameskip_threshold(state, -1);
        if (pressed & PAD_RIGHT)
            cycle_frameskip_threshold(state, 1);
        if ((pressed & PAD_START) || (pressed & PAD_CROSS))
            cycle_frameskip_threshold(state, 1);
    }

    if (pressed & PAD_CIRCLE)
        state->page = SELECT_MENU_PAGE_MAIN;
}
