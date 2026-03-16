    .section .rodata
    .align 4
    .global smw_sfc_start
    .global smw_sfc_end

smw_sfc_start:
    .incbin "smw.sfc"
smw_sfc_end:
