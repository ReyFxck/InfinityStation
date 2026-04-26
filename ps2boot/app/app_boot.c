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

/* INF: símbolos exportados por ps2boot/irx/usb_irx_blob.S */
extern unsigned char _usbd_irx_start[];
extern unsigned char _usbd_irx_end[];
extern unsigned char _bdm_irx_start[];
extern unsigned char _bdm_irx_end[];
extern unsigned char _usbmass_bd_irx_start[];
extern unsigned char _usbmass_bd_irx_end[];
extern unsigned char _bdmfs_fatfs_irx_start[];
extern unsigned char _bdmfs_fatfs_irx_end[];

static int app_boot_load_irx_blob(unsigned char *start, unsigned char *end, const char *name)
{
    int rc, ret = 0;
    int size = (int)(end - start);
    if (size <= 0) return 0;
    rc = SifExecModuleBuffer(start, (unsigned)size, 0, NULL, &ret);
    (void)name;
    return (rc >= 0 && ret >= 0) ? 1 : 0;
}

static void app_boot_load_usb_modules(void)
{
    (void)app_boot_load_irx_blob(_usbd_irx_start, _usbd_irx_end, "usbd.irx");
    (void)app_boot_load_irx_blob(_bdm_irx_start, _bdm_irx_end, "bdm.irx");
    (void)app_boot_load_irx_blob(_usbmass_bd_irx_start, _usbmass_bd_irx_end, "usbmass_bd.irx");
    (void)app_boot_load_irx_blob(_bdmfs_fatfs_irx_start, _bdmfs_fatfs_irx_end, "bdmfs_fatfs.irx");
    /* tempo para o stack USB enumerar antes de outros módulos. */
    {
        int i;
        for (i = 0; i < 60; i++)
            (void)SifIopSync();
    }
}
static void app_boot_reset_iop_minimal(void)
{


    SifExitRpc();
    SifInitRpc(0);

    while (!SifIopReset(NULL, 0)) { }

    while (!SifIopSync()) { }

    SifInitRpc(0);
    SifInitIopHeap();
    SifLoadFileInit();

    (void)sbv_patch_enable_lmb();

    (void)sbv_patch_disable_prefix_check();
}

void app_boot_init(void (*die_fn)(const char *msg))
{

    SifInitRpc(0);

    app_boot_reset_iop_minimal();
    app_boot_load_usb_modules();
    ps2_audio_set_iop_ready(1);
    ps2_disc_init_once();

    if (!ps2_video_init_once())
        die_fn("ps2_video_init_once() falhou");

    if (!ps2_audio_init_once())
        die_fn("ps2_audio_init_once() falhou");

    (void)ps2_input_init_once();

    ps2_menu_init();

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
