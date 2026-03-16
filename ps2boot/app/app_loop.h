#ifndef APP_LOOP_H
#define APP_LOOP_H

#include <stdint.h>
#include "libretro.h"

void app_loop_run_launcher(uint32_t *prev_buttons);

int app_loop_handle_menu(uint32_t buttons,
                         uint32_t pressed,
                         uint32_t *prev_buttons,
                         struct retro_system_av_info *av,
                         int *saved_launcher_x,
                         int *saved_launcher_y,
                         void (*die_fn)(const char *msg));

#endif
