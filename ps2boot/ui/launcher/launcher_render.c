#include "launcher_render.h"

#include "../../ps2_video.h"
#include "launcher_pages.h"

void launcher_render(const launcher_state_t *state)
{
    if (!state)
        return;

    ps2_video_menu_begin_frame();
    launcher_pages_draw(state);
    ps2_video_menu_end_frame();
}
