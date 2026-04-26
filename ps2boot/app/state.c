#include "state.h"

static app_mode_t g_app_mode = APP_MODE_BOOT;
static app_request_t g_app_request = APP_REQUEST_NONE;

void app_state_init(void)
{
    g_app_mode = APP_MODE_BOOT;
    g_app_request = APP_REQUEST_NONE;
}

app_mode_t app_state_mode(void)
{
    return g_app_mode;
}

void app_state_set_mode(app_mode_t mode)
{
    g_app_mode = mode;
}

void app_state_request(app_request_t req)
{
    g_app_request = req;
}

app_request_t app_state_take_request(void)
{
    app_request_t req = g_app_request;
    g_app_request = APP_REQUEST_NONE;
    return req;
}
