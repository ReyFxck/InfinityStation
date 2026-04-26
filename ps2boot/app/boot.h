#ifndef APP_BOOT_H
#define APP_BOOT_H

#include <stdint.h>
#include "libretro.h"

void app_boot_init(void (*die_fn)(const char *msg));
void app_boot_log_core_info(void);
void app_boot_run_launcher(uint32_t *prev_buttons, int *saved_launcher_x, int *saved_launcher_y);
void app_boot_refresh_av_info(struct retro_system_av_info *av);

#endif
