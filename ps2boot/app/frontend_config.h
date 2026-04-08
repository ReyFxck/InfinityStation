#ifndef FRONTEND_CONFIG_H
#define FRONTEND_CONFIG_H

typedef struct {
    int aspect;
    int display_x;
    int display_y;
    int show_fps;
    int fps_rainbow;
    int frame_limit;
    int game_vsync;
    int game_reduce_slowdown;
    int game_reduce_flicker;
    int game_frameskip_mode;
    int game_frameskip_threshold;
} frontend_config_t;

void frontend_config_init_defaults(void);
const frontend_config_t *frontend_config_get(void);
frontend_config_t *frontend_config_mut(void);
void frontend_config_apply(void);

void frontend_config_set_aspect(int aspect);
void frontend_config_set_display(int x, int y);
void frontend_config_set_show_fps(int enabled);
void frontend_config_set_fps_rainbow(int enabled);
void frontend_config_set_frame_limit(int mode);
void frontend_config_set_game_vsync(int enabled);
void frontend_config_set_game_reduce_slowdown(int mode);
void frontend_config_set_game_reduce_flicker(int enabled);
void frontend_config_set_game_frameskip_mode(int mode);
void frontend_config_set_game_frameskip_threshold(int threshold);

#endif
