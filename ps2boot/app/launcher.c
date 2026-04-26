#include "launcher.h"

#include <stdint.h>

#include "input.h"
#include "ui/launcher/launcher.h"

static int g_launcher_bootstrapped = 0;

void app_launcher_run(uint32_t *prev_buttons)
{
    uint32_t prev = 0;
    int input_armed = 0;

    if (prev_buttons)
        *prev_buttons = 0;

    if (!g_launcher_bootstrapped) {
        launcher_init();
        g_launcher_bootstrapped = 1;
    }

    launcher_clear_start_request();

    while (!launcher_should_start_game()) {
        uint32_t buttons;
        uint32_t pressed;

        ps2_input_poll();
        buttons = ps2_input_buttons();

        /* Evita reabrir instantaneamente o jogo ao voltar do EXIT GAME.
           O launcher só arma depois que todos os botões forem soltos. */
        if (!input_armed) {
            launcher_draw();

            if (buttons == 0) {
                input_armed = 1;
                prev = 0;
            } else {
                prev = buttons;
            }

            continue;
        }

        pressed = buttons & ~prev;

        launcher_handle(pressed);
        launcher_draw();

        prev = buttons;
    }

    launcher_clear_start_request();

    if (prev_buttons)
        *prev_buttons = prev;
}
