#ifndef SELECT_MENU_STATE_H
#define SELECT_MENU_STATE_H

enum {
    SELECT_MENU_PAGE_MAIN = 0,
    SELECT_MENU_PAGE_VIDEO = 1,
    SELECT_MENU_PAGE_VIDEO_DISPLAY = 2,
    SELECT_MENU_PAGE_VIDEO_ASPECT = 3,
    SELECT_MENU_PAGE_GAME_OPTIONS = 4
};

enum {
    SELECT_MENU_FRAME_LIMIT_AUTO = 0,
    SELECT_MENU_FRAME_LIMIT_50 = 1,
    SELECT_MENU_FRAME_LIMIT_60 = 2,
    SELECT_MENU_FRAME_LIMIT_OFF = 3
};

enum {
    SELECT_MENU_ACTION_NONE = 0,
    SELECT_MENU_ACTION_RESTART = 1,
    SELECT_MENU_ACTION_OPEN_LAUNCHER = 2
};

typedef struct {
    int open;
    int page;
    int main_sel;
    int video_sel;
    int aspect_sel;
    int game_sel;
    int show_fps;
    int fps_rainbow;
    int frame_limit;
    int game_vsync;
    int pending_action;
} select_menu_state_t;

void select_menu_state_reset(void);
const select_menu_state_t *select_menu_state_get(void);
select_menu_state_t *select_menu_state_mut(void);

#endif
