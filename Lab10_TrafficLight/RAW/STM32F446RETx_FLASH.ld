/*
 * STM32F446RETx_FLASH.ld — Lab 10 Traffic Light STM32 (bare-metal RAW)
 *
 * Identical to the linker script used by all other RAW labs (Lab11–Lab15).
 * STM32F446RE memory map:
 *   Flash: 512 KB starting at 0x08000000
 *   SRAM1: 128 KB starting at 0x20000000
 */
ENTRY(Reset_Handler)

_estack = ORIGIN(RAM) + LENGTH(RAM);   /* top of RAM = initial stack pointer */

MEMORY {
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS {
    /* Vector table + code + read-only data → Flash */
    .isr_vector : { . = ALIGN(4); KEEP(*(.isr_vector)) . = ALIGN(4); } >FLASH
    .text       : { . = ALIGN(4); *(.text) *(.text*) *(.rodata) *(.rodata*) . = ALIGN(4); _etext = .; } >FLASH

    /* Initialised data: stored in Flash, copied to RAM by Reset_Handler */
    _sidata = LOADADDR(.data);
    .data : { . = ALIGN(4); _sdata = .; *(.data) *(.data*) . = ALIGN(4); _edata = .; } >RAM AT>FLASH

    /* Zero-initialised data: lives only in RAM, zeroed by Reset_Handler */
    .bss  : { . = ALIGN(4); _sbss = .; *(.bss) *(.bss*) *(COMMON) . = ALIGN(4); _ebss = .; } >RAM

    /DISCARD/ : { libc.a(*) libm.a(*) libgcc.a(*) }
}
