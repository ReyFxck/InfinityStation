#ifndef SELECT_MENU_RENDER_H
#define SELECT_MENU_RENDER_H

#include "state.h"

typedef struct {
    int display_x;
    int display_y;
    int current_aspect;
} select_menu_view_state_t;

void select_menu_render(const select_menu_state_t *state,
                        const select_menu_view_state_t *view_state);

#endif
