@ startup.s — Lab 10 Traffic Light STM32 (bare-metal RAW version)
@
@ No interrupt handlers are required.
@ The FSM uses SysTick in polling mode only (reads COUNTFLAG from STK_CTRL);
@ the SysTick interrupt line is never enabled, so SysTick_Handler → Default.
@
@ Vector table layout (STM32F446RE, RM0390 §10.2):
@   0        → _estack       (initial stack pointer)
@   1        → Reset_Handler (program entry)
@   2–14     → Default_Handler (fault/system exceptions)
@   15       → Default_Handler (SysTick — polled, not interrupt-driven)
@   16–106   → 0              (IRQ0–IRQ90, no peripherals use interrupts)
@
@ Consistent with startup.s used in Lab11–Lab15.

    .syntax unified
    .cpu cortex-m4
    .thumb

@ ---------------------------------------------------------------------------
@ Vector table — must be placed at 0x08000000 (start of Flash)
@ ---------------------------------------------------------------------------
    .section .isr_vector, "a", %progbits
g_pfnVectors:
    .word   _estack                  @ 0:  initial stack pointer (top of RAM)
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
    .word   Default_Handler          @ 15: SysTick (polled only — no ISR needed)
    @ IRQ0–IRQ90: no peripheral interrupts used in this lab
    .fill   91, 4, 0

@ ---------------------------------------------------------------------------
@ Reset_Handler
@   1. Zero-initialise .bss  (uninitialized global/static variables)
@   2. Copy     .data from Flash load address to RAM run address
@   3. Branch to main()
@
@ Symbol names (_sbss, _ebss, _sdata, _edata, _sidata) are defined in
@ STM32F446RETx_FLASH.ld — identical linker script used by all RAW labs.
@ ---------------------------------------------------------------------------
    .section .text.Reset_Handler, "ax", %progbits
    .weak   Reset_Handler
    .type   Reset_Handler, %function
Reset_Handler:
    @ --- zero .bss ---
    ldr     r0, =_sbss
    ldr     r1, =_ebss
    movs    r2, #0
bss_loop:
    cmp     r0, r1
    bge     bss_done
    str     r2, [r0], #4
    b       bss_loop
bss_done:
    @ --- copy .data from Flash → RAM ---
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
    b       .                        @ should never return; loop if it does
    .size   Reset_Handler, .-Reset_Handler

@ ---------------------------------------------------------------------------
@ Default_Handler — catches any unexpected exception/interrupt
@ Infinite loop so the fault is visible in a debugger (PC stuck here)
@ ---------------------------------------------------------------------------
    .section .text.Default_Handler, "ax", %progbits
    .weak   Default_Handler
    .type   Default_Handler, %function
Default_Handler:
    b       .
    .size   Default_Handler, .-Default_Handler
