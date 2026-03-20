#ifndef LAUNCHER_BROWSER_H
#define LAUNCHER_BROWSER_H

#include <stddef.h>

typedef struct
{
    char name[128];
    int is_dir;
} launcher_browser_entry_t;

void launcher_browser_init(void);
int launcher_browser_open(const char *path);
int launcher_browser_refresh(void);
int launcher_browser_go_parent(void);

int launcher_browser_count(void);
int launcher_browser_selected(void);
int launcher_browser_scroll(void);
const char *launcher_browser_current_path(void);
const launcher_browser_entry_t *launcher_browser_entry(int index);
int launcher_browser_last_error(void);
int launcher_browser_device_ready(const char *name);
void launcher_browser_refresh_root_device_statuses(void);
void launcher_browser_clear_error(void);

void launcher_browser_move(int delta, int visible_rows);
int launcher_browser_activate(char *selected_path, size_t path_size, char *selected_label, size_t label_size);

#endif
