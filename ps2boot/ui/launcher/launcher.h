#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <stdint.h>

void launcher_init(void);
void launcher_handle(uint32_t pressed);
void launcher_draw(void);
int launcher_should_start_game(void);
void launcher_clear_start_request(void);
const char *launcher_selected_path(void);
const char *launcher_selected_label(void);

#endif
