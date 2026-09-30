@ startup.s — Lab 16 IoT STM32 Bare-Metal Port
@ Cortex-M4 startup code with vector table and reset handler

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
    @ IRQ 0..90
    .fill   91, 4, 0

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
    @ Enable FPU (Coprocessors CP10 and CP11 Full Access)
    ldr.w   r0, =0xE000ED88          @ CPACR address
    ldr     r1, [r0]
    orr     r1, r1, #(0xF << 20)     @ CP10 and CP11 full access
    str     r1, [r0]
    dsb
    isb

    @ Call main()
    bl      main
    b       .
    .size   Reset_Handler, .-Reset_Handler

    @ -----------------------------------------------------------------------
    .section .text.Default_Handler, "ax", %progbits
Default_Handler:
    b       .
    .size   Default_Handler, .-Default_Handler

    .weak   SysTick_Handler
    .thumb_set SysTick_Handler, Default_Handler
