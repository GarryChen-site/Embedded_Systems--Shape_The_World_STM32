

    .syntax unified
    .cpu cortex-m4
    .thumb

    .section .isr_vector, "a", %progbits
    .type g_pfnVectors, %object
    .size g_pfnVectors, .-g_pfnVectors

g_pfnVectors:
    .word    _estack
    .word    Reset_Handler

    .word    Default_Handler
    .word    Default_Handler
    .word    Default_Handler
    .word    Default_Handler
    .word    Default_Handler
    .word    0
    .word    0
    .word    0
    .word    0
    .word    Default_Handler
    .word    Default_Handler
    .word    0
    .word    Default_Handler
    .word    Default_Handler

    .fill    91, 4, 0


    .section .text.Reset_Handler, "ax", %progbits
    .weak   Reset_Handler
    .type   Reset_Handler, %function

Reset_Handler:

    ldr     r0, =_sbss
    ldr     r1, =_ebss
    movs    r2, #0

bss_loop:
    cmp     r0, r1
    bge     bss_done
    str     r2, [r0], #4
    b       bss_loop

bss_done:
    ldr     r0, =_sdata
    ldr     r1, =_edata
    ldr     r2, =_sidata

data_loop:
    cmp     r0, r1
    bge     data_done
    ldr     r3, [r2], #4
    std     r3, [r0], #4
    b       data_loop

data_done:
    bl      main
    b       .
    .size   Reset_Handler, .-Reset_Handler

    .section .text.Default_Handler, "ax", %progbits
    .weak   Default_Handler
    .type   Default_handler, %function

Default_Handler:
    b       .
    .size   Default_Handler, .-Default_Handler

