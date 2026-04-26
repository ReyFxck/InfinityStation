#include "select_menu_actions_internal.h"
#include "frontend_config.h"

#include "video.h"

void select_menu_actions_handle_video(uint32_t pressed)
{
    select_menu_state_t *state = select_menu_state_mut();
    int x;
    int y;
    int aspect = -1;

    if (state->page == SELECT_MENU_PAGE_VIDEO) {
        if (pressed & PAD_UP)
            state->video_sel = select_menu_wrap_index(state->video_sel - 1, 3);

        if (pressed & PAD_DOWN)
            state->video_sel = select_menu_wrap_index(state->video_sel + 1, 3);

        if (pressed & (PAD_START | PAD_CROSS)) {
            if (state->video_sel == 0) {
                state->page = SELECT_MENU_PAGE_VIDEO_DISPLAY;
            } else if (state->video_sel == 1) {
                state->page = SELECT_MENU_PAGE_VIDEO_ASPECT;
                state->aspect_sel = ps2_video_get_aspect();
            } else {
                state->page = SELECT_MENU_PAGE_MAIN;
            }
        }

        if (pressed & PAD_CIRCLE)
            state->page = SELECT_MENU_PAGE_MAIN;
        return;
    }

    if (state->page == SELECT_MENU_PAGE_VIDEO_DISPLAY) {
        ps2_video_get_offsets(&x, &y);

        if (pressed & PAD_LEFT)  x -= 8;
        if (pressed & PAD_RIGHT) x += 8;
        if (pressed & PAD_UP)    y -= 8;
        if (pressed & PAD_DOWN)  y += 8;

        ps2_video_set_offsets(x, y);
        frontend_config_set_display(x, y);

        if (pressed & (PAD_START | PAD_CROSS | PAD_CIRCLE))
            state->page = SELECT_MENU_PAGE_VIDEO;
        return;
    }

    if (state->page == SELECT_MENU_PAGE_VIDEO_ASPECT) {
        if (pressed & PAD_UP)
            state->aspect_sel = select_menu_wrap_index(state->aspect_sel - 1, 5);

        if (pressed & PAD_DOWN)
            state->aspect_sel = select_menu_wrap_index(state->aspect_sel + 1, 5);

        if (pressed & (PAD_START | PAD_CROSS)) {
            if (state->aspect_sel == 0)
                aspect = PS2_ASPECT_4_3;
            else if (state->aspect_sel == 1)
                aspect = PS2_ASPECT_16_9;
            else if (state->aspect_sel == 2)
                aspect = PS2_ASPECT_FULL;
            else if (state->aspect_sel == 3)
                aspect = PS2_ASPECT_PIXEL;

            if (aspect >= 0) {
                ps2_video_set_aspect(aspect);
                frontend_config_set_aspect(aspect);
            }

            state->page = SELECT_MENU_PAGE_VIDEO;
        }

        if (pressed & PAD_CIRCLE)
            state->page = SELECT_MENU_PAGE_VIDEO;
        return;
    }
}
