#include "frontend_config.h"
#include "ps2_video.h"

static frontend_config_t g_frontend_config;
static int g_frontend_config_inited = 0;

void frontend_config_init_defaults(void)
{
    if (g_frontend_config_inited)
        return;

    g_frontend_config.aspect = PS2_ASPECT_4_3;
    g_frontend_config.display_x = 0;
    g_frontend_config.display_y = 0;
    g_frontend_config.show_fps = 0;
    g_frontend_config.fps_rainbow = 0;
    g_frontend_config.frame_limit = 3; /* SELECT_MENU_FRAME_LIMIT_OFF */
    g_frontend_config.game_vsync = 0;
    g_frontend_config.game_reduce_slowdown = 0;
    g_frontend_config.game_reduce_flicker = 0;
    g_frontend_config.game_frameskip_mode = 0;
    g_frontend_config.game_frameskip_threshold = 33;
    g_frontend_config_inited = 1;
}

const frontend_config_t *frontend_config_get(void)
{
    return &g_frontend_config;
}

frontend_config_t *frontend_config_mut(void)
{
    return &g_frontend_config;
}

void frontend_config_apply(void)
{
    ps2_video_set_aspect(g_frontend_config.aspect);
    ps2_video_set_offsets(g_frontend_config.display_x, g_frontend_config.display_y);
}

void frontend_config_set_aspect(int aspect)
{
    g_frontend_config.aspect = aspect;
}

void frontend_config_set_display(int x, int y)
{
    g_frontend_config.display_x = x;
    g_frontend_config.display_y = y;
}

void frontend_config_set_show_fps(int enabled)
{
    g_frontend_config.show_fps = enabled ? 1 : 0;
    if (!g_frontend_config.show_fps)
        g_frontend_config.fps_rainbow = 0;
}

void frontend_config_set_fps_rainbow(int enabled)
{
    g_frontend_config.fps_rainbow = enabled ? 1 : 0;
}

void frontend_config_set_frame_limit(int mode)
{
    g_frontend_config.frame_limit = mode;
}

void frontend_config_set_game_vsync(int enabled)
{
    g_frontend_config.game_vsync = enabled ? 1 : 0;
}

void frontend_config_set_game_reduce_slowdown(int mode)
{
    if (mode < 0)
        mode = 0;
    if (mode > 2)
        mode = 2;
    g_frontend_config.game_reduce_slowdown = mode;
}

void frontend_config_set_game_reduce_flicker(int enabled)
{
    g_frontend_config.game_reduce_flicker = enabled ? 1 : 0;
}

void frontend_config_set_game_frameskip_mode(int mode)
{
    if (mode < 0)
        mode = 0;
    if (mode > 2)
        mode = 2;
    g_frontend_config.game_frameskip_mode = mode;
}

void frontend_config_set_game_frameskip_threshold(int threshold)
{
    if (threshold < 15)
        threshold = 15;
    if (threshold > 60)
        threshold = 60;
    g_frontend_config.game_frameskip_threshold = threshold;
}
