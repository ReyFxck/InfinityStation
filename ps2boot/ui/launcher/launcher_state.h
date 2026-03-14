#ifndef LAUNCHER_STATE_H
#define LAUNCHER_STATE_H

enum
{
    LAUNCHER_PAGE_MAIN = 0,
    LAUNCHER_PAGE_BROWSER = 1,
    LAUNCHER_PAGE_OPTIONS = 2
};

typedef struct
{
    int page;
    int main_sel;
    int should_start_game;
    char selected_path[256];
    char selected_label[128];
} launcher_state_t;

#endif
