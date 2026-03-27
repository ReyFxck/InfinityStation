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

static void app_boot_reset_iop_minimal(void)
{
    int r;

    scr_printf("[BOOT] iop reset: begin\n");

    SifExitRpc();
    SifInitRpc(0);

    while (!SifIopReset(NULL, 0)) { }
    scr_printf("[BOOT] iop reset: requested\n");

    while (!SifIopSync()) { }
    scr_printf("[BOOT] iop reset: synced\n");

    SifInitRpc(0);
    SifInitIopHeap();
    SifLoadFileInit();
    scr_printf("[BOOT] iop reset: services ready\n");

    r = sbv_patch_enable_lmb();
    scr_printf("[BOOT] sbv_patch_enable_lmb -> %d\n", r);

    r = sbv_patch_disable_prefix_check();
    scr_printf("[BOOT] sbv_patch_disable_prefix_check -> %d\n", r);
}

void app_boot_init(void (*die_fn)(const char *msg))
{
    scr_printf("[BOOT] app_boot_init: enter\n");

    SifInitRpc(0);
    scr_printf("[BOOT] app_boot_init: after SifInitRpc\n");

    app_boot_reset_iop_minimal();

  scr_printf("[BOOT] app_boot_init: before ps2_video_init_once\n");
    if (!ps2_video_init_once())
        die_fn("ps2_video_init_once() falhou");
    scr_printf("[BOOT] app_boot_init: after ps2_video_init_once\n");

    printf("[AUDIO] boot init begin\n");
    printf("[BOOTP] before ps2_audio_init_once\n");
    if (!ps2_audio_init_once()) {
        printf("[AUDIO] boot init FAILED, continuing without audio\n");
        printf("[BOOTP] after ps2_audio_init_once FAIL\n");
    } else {
        printf("[AUDIO] boot init OK\n");
        printf("[BOOTP] after ps2_audio_init_once OK\n");
    }

    printf("[BOOTP] before ps2_input_init_once\n");
    (void)ps2_input_init_once();
    printf("[BOOTP] after ps2_input_init_once\n");

    printf("[BOOTP] before ps2_menu_init\n");
    ps2_menu_init();
    printf("[BOOTP] after ps2_menu_init\n");

    printf("[BOOTP] app_boot_init return\n");
}

void app_boot_log_core_info(void)
{
    struct retro_system_info info;

    memset(&info, 0, sizeof(info));
    retro_get_system_info(&info);

    scr_printf("core: %s %s\n",
        info.library_name ? info.library_name : "(null)",
        info.library_version ? info.library_version : "(null)");

    scr_printf("need_fullpath=%d block_extract=%d valid_ext=%s\n",
        info.need_fullpath,
        info.block_extract,
        info.valid_extensions ? info.valid_extensions : "(null)");
}

void app_boot_run_launcher(uint32_t *prev_buttons, int *saved_launcher_x, int *saved_launcher_y)
{
    (void)prev_buttons;
    *saved_launcher_x = 0;
    *saved_launcher_y = 0;
    printf("[BOOT] launcher bypass debug\n");
}

void app_boot_refresh_av_info(struct retro_system_av_info *av)
{
    memset(av, 0, sizeof(*av));
    retro_get_system_av_info(av);
}
