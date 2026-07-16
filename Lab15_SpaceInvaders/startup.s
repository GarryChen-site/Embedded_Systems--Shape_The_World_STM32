    .syntax unified
    .cpu cortex-m4
    .thumb

    .section .isr_vector, "a", %progbits
g_pfnVectors:
    .word   _estack                  @ 0:  initial stack pointer
    .word   Reset_Handler            @ 1:  reset
    .word   Default_Handler          @ 2:  NMI
    .word   Default_Handler          @ 3:  HardFault
    .word   Default_Handler          @ 4:  MemManage
    .word   Default_Handler          @ 5:  BusFault
    .word   Default_Handler          @ 6:  UsageFault
    .word   0                        @ 7:  reserved
    .word   0                        @ 8:  reserved
    .word   0                        @ 9:  reserved
    .word   0                        @ 10: reserved
    .word   Default_Handler          @ 11: SVCall
    .word   Default_Handler          @ 12: Debug Monitor
    .word   0                        @ 13: reserved
    .word   Default_Handler          @ 14: PendSV
    .word   SysTick_Handler          @ 15: SysTick
    @ IRQ0-22 (23 words of 0)
    .fill   23, 4, 0
    @ IRQ23 = EXTI9_5 (PC6 missile, PC8 laser)
    .word   EXTI9_5_IRQHandler
    @ IRQ24-90 (67 words of 0)
    .fill   67, 4, 0

    @ -----------------------------------------------------------------------
    .section .text.Reset_Handler, "ax", %progbits
    .weak   Reset_Handler
    .type   Reset_Handler, %function
Reset_Handler:
    @ Zero-fill .bss
    ldr     r0, =_sbss
    ldr     r1, =_ebss
    movs    r2, #0
bss_loop:
    cmp     r0, r1
    bge     bss_done
    str     r2, [r0], #4
    b       bss_loop
bss_done:
    @ Copy .data from Flash to RAM
    ldr     r0, =_sdata
    ldr     r1, =_edata
    ldr     r2, =_sidata
data_loop:
    cmp     r0, r1
    bge     data_done
    ldr     r3, [r2], #4
    str     r3, [r0], #4
    b       data_loop
data_done:
    bl      main
    b       .
    .size   Reset_Handler, .-Reset_Handler

    @ -----------------------------------------------------------------------
    .section .text.Default_Handler, "ax", %progbits
    .weak   Default_Handler
    .type   Default_Handler, %function
Default_Handler:
    b       .
    .size   Default_Handler, .-Default_Handler

    .weak   SysTick_Handler
    .thumb_set SysTick_Handler, Default_Handler

    .weak   EXTI9_5_IRQHandler
    .thumb_set EXTI9_5_IRQHandler, Default_Handler