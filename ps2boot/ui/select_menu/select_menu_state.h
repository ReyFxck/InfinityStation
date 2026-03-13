#ifndef SELECT_MENU_STATE_H
#define SELECT_MENU_STATE_H

enum
{
    SELECT_MENU_PAGE_MAIN = 0,
    SELECT_MENU_PAGE_VIDEO = 1,
    SELECT_MENU_PAGE_VIDEO_DISPLAY = 2,
    SELECT_MENU_PAGE_VIDEO_ASPECT = 3
};

typedef struct
{
    int open;
    int page;
    int main_sel;
    int video_sel;
    int aspect_sel;
} select_menu_state_t;

#endif
