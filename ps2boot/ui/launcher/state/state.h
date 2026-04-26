#ifndef LAUNCHER_STATE_H
#define LAUNCHER_STATE_H

#include "common/inf_paths.h"

enum {
    LAUNCHER_PAGE_MAIN = 0,
    LAUNCHER_PAGE_BROWSER = 1,
    LAUNCHER_PAGE_OPTIONS = 2,
    LAUNCHER_PAGE_CREDITS = 3
};

typedef struct {
    int page;
    int main_sel;
    int should_start_game;
    char selected_path[INF_PATH_MAX];
    char selected_label[INF_LABEL_MAX];
} launcher_state_t;

void launcher_state_reset(void);
const launcher_state_t *launcher_state_get(void);
launcher_state_t *launcher_state_mut(void);

#endif
