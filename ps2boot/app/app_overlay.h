#ifndef APP_OVERLAY_H
#define APP_OVERLAY_H

void app_overlay_reset_timing(void);
void app_overlay_set_core_nominal_fps(double fps);
double app_overlay_get_core_nominal_fps(void);
void app_overlay_update_fps(void);
void app_overlay_throttle_if_needed(void);
unsigned app_overlay_video_wait_vblanks(int game_vsync_enabled);

#endif
