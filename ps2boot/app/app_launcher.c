#include "app_launcher.h"

#include <stdint.h>

#include "ps2_input.h"
#include "ui/launcher/launcher.h"

void app_launcher_run(uint32_t *prev_buttons)
{
    uint32_t prev = 0;

    if (prev_buttons)
        *prev_buttons = 0;

    launcher_init();

    while (!launcher_should_start_game()) {
        uint32_t buttons;
        uint32_t pressed;

        ps2_input_poll();
        buttons = ps2_input_buttons();
        pressed = buttons & ~prev;

        launcher_handle(pressed);
        launcher_draw();

        prev = buttons;
    }

    launcher_clear_start_request();

    if (prev_buttons)
        *prev_buttons = 0;
}
