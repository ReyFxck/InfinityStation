    .section .rodata
    .align 16
    .global _audsrv_irx_start
    .global _audsrv_irx_end
_audsrv_irx_start:
    .incbin "/root/ps2dev/ps2sdk/iop/irx/audsrv.irx"
_audsrv_irx_end:
