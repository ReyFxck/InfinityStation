    .section .rodata
    .align 16
    .global _usbd_irx_start
    .global _usbd_irx_end
_usbd_irx_start:
    .incbin "/usr/local/ps2dev/ps2sdk/iop/irx/usbd.irx"
_usbd_irx_end:
    .global _bdm_irx_start
    .global _bdm_irx_end
_bdm_irx_start:
    .incbin "/usr/local/ps2dev/ps2sdk/iop/irx/bdm.irx"
_bdm_irx_end:
    .global _usbmass_bd_irx_start
    .global _usbmass_bd_irx_end
_usbmass_bd_irx_start:
    .incbin "/usr/local/ps2dev/ps2sdk/iop/irx/usbmass_bd.irx"
_usbmass_bd_irx_end:
    .global _bdmfs_fatfs_irx_start
    .global _bdmfs_fatfs_irx_end
_bdmfs_fatfs_irx_start:
    .incbin "/usr/local/ps2dev/ps2sdk/iop/irx/bdmfs_fatfs.irx"
_bdmfs_fatfs_irx_end:
