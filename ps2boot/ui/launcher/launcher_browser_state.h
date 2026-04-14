#ifndef LAUNCHER_BROWSER_STATE_H
#define LAUNCHER_BROWSER_STATE_H

#include "launcher_browser.h"
#include "common/inf_paths.h"

#include <dirent.h>

typedef struct {
    launcher_browser_entry_t *entries;
    int entry_count;
    int entry_capacity;
    char current_path[INF_PATH_MAX];
    int selected;
    int scroll;
    int last_error;
    DIR *scan_dir;
    int scan_done;
} launcher_browser_state_t;

void launcher_browser_state_reset(void);
const launcher_browser_state_t *launcher_browser_state_get(void);
launcher_browser_state_t *launcher_browser_state_mut(void);

#endif
