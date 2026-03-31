#ifndef APP_TRANSITION_H
#define APP_TRANSITION_H

#include <stdint.h>

struct retro_system_av_info;

void app_transition_prepare(uint32_t *prev_buttons);
void app_transition_load_selected_game(struct retro_system_av_info *av,
                                       void (*die_fn)(const char *msg));
void app_transition_restart_game(struct retro_system_av_info *av,
                                 uint32_t *prev_buttons,
                                 void (*die_fn)(const char *msg));
void app_transition_open_launcher_and_reload(struct retro_system_av_info *av,
                                             uint32_t *prev_buttons,
                                             int *saved_launcher_x,
                                             int *saved_launcher_y,
                                             void (*die_fn)(const char *msg));

#endif
