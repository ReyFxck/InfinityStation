#ifndef APP_RUNTIME_H
#define APP_RUNTIME_H

#include <stdint.h>
#include "libretro.h"

int app_runtime_handle_menu(uint32_t buttons,
                            uint32_t pressed,
                            uint32_t *prev_buttons,
                            struct retro_system_av_info *av,
                            int *saved_launcher_x,
                            int *saved_launcher_y,
                            void (*die_fn)(const char *msg));

#endif
