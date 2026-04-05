#include "app_boot.h"
#include <stdio.h>
#include "app_launcher.h"

#include <debug.h>
#include <sifrpc.h>
#include <iopcontrol.h>
#include <iopheap.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <string.h>

#include "libretro.h"
#include "ps2_video.h"
#include "ps2_input.h"
#include "ps2_menu.h"
#include "ps2_audio.h"
#include "ps2_disc.h"

static void app_boot_reset_iop_minimal(void)
{
    int r;

    /* log removido */

    SifExitRpc();
    SifInitRpc(0);

    while (!SifIopReset(NULL, 0)) { }
    /* log removido */

    while (!SifIopSync()) { }
    /* log removido */

    SifInitRpc(0);
    SifInitIopHeap();
    SifLoadFileInit();
    /* log removido */

    r = sbv_patch_enable_lmb();
    /* log removido */

    r = sbv_patch_disable_prefix_check();
    /* log removido */
}

void app_boot_init(void (*die_fn)(const char *msg))
{
    /* log removido */

    SifInitRpc(0);
    /* log removido */

    app_boot_reset_iop_minimal();
    ps2_disc_init_once();

  /* log removido */
    if (!ps2_video_init_once())
        die_fn("ps2_video_init_once() falhou");
    /* log removido */

    /* log removido */
    /* log removido */
    if (!ps2_audio_init_once()) {
        /* log removido */
        /* log removido */
    } else {
        /* log removido */
        /* log removido */
    }

    /* log removido */
    (void)ps2_input_init_once();
    /* log removido */

    /* log removido */
    ps2_menu_init();
    /* log removido */

    /* log removido */
}

void app_boot_log_core_info(void)
{
    struct retro_system_info info;

    memset(&info, 0, sizeof(info));
    retro_get_system_info(&info);

    

}

void app_boot_run_launcher(uint32_t *prev_buttons, int *saved_launcher_x, int *saved_launcher_y)
{
    ps2_video_get_offsets(saved_launcher_x, saved_launcher_y);
    ps2_video_set_offsets(0, 0);
    app_launcher_run(prev_buttons);
    ps2_video_set_offsets(*saved_launcher_x, *saved_launcher_y);
}

void app_boot_refresh_av_info(struct retro_system_av_info *av)
{
    memset(av, 0, sizeof(*av));
    retro_get_system_av_info(av);
}
