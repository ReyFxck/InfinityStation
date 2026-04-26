#ifndef APP_STATE_H
#define APP_STATE_H

typedef enum app_mode_e {
    APP_MODE_BOOT = 0,
    APP_MODE_LAUNCHER,
    APP_MODE_GAME,
    APP_MODE_MENU
} app_mode_t;

typedef enum app_request_e {
    APP_REQUEST_NONE = 0,
    APP_REQUEST_RESTART_GAME,
    APP_REQUEST_OPEN_LAUNCHER
} app_request_t;

void app_state_init(void);

app_mode_t app_state_mode(void);
void app_state_set_mode(app_mode_t mode);

void app_state_request(app_request_t req);
app_request_t app_state_take_request(void);

#endif
