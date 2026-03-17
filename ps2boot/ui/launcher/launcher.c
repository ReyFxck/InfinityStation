#include "launcher.h"

#include "launcher_actions.h"
#include "launcher_render.h"

void launcher_init(void)
{
    launcher_actions_init();
}

void launcher_handle(uint32_t pressed)
{
    launcher_actions_handle(pressed);
}

void launcher_draw(void)
{
    launcher_render(launcher_actions_state());
}

int launcher_should_start_game(void)
{
    return launcher_actions_should_start_game();
}

void launcher_clear_start_request(void)
{
    launcher_actions_clear_start_request();
}

const char *launcher_selected_path(void)
{
    return launcher_actions_selected_path();
}

const char *launcher_selected_label(void)
{
    return launcher_actions_selected_label();
}
