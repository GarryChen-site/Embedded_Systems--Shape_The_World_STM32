
ENTRY(Reset_Handler)


_estack = ORIGIN(RAM) + LENGTH(RAM);

MEMORY {

    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 512k

    RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 128k
}

SECTIONS {
    .isr_vector : {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } >FLASH

    .text : {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        . = ALIGN(4);
    } >FLASH

    .rodata : {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } >FLASH

    /* When GCC compiles any function, it may generate stack-unwinding metadata
     * so that if an exception (or a fault handler) needs to walk back up the call
     * stack, it knows how to undo each stack frame. 
     * the ARM EABI requires these tables to be present and reachable in Flash
     */

    .ARM.extab : {
        . = ALIGN(4);
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        . = ALIGN(4);
    } >FLASH

    .ARM : {
        . = ALIGN(4);
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
        . = ALIGN(4);
    } >FLASH

    .preinit_array : {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array*))
        PROVIDE_HIDDEN (__preinit_array_end = .);
        . = ALIGN(4);
    } >FLASH
    
    /* An array of function pointers that GCC places in Flash. A startup
     * routine iterates through this array and calls each function before
     * main() runs. In C++ it holds static constructors. In C it drives:
     * - Functions marked __attribute__((constructor))
     * - newlib's __libc_init_array()  
     * Newlib's printf needs the FILE structures (stdout, stdin, stderr) to be
     * initialized before the first call. Newlib registers this initialization in
     * .init_array. The startup code is supposed to call __libc_init_array()
     * which iterates the array.
     */

    .init_array : {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
        . = ALIGN(4);
    } >FLASH

    /* The mirror image of .init_array. Called by __libc_fini_array()
     * after main() returns (via exit()). In C++ it holds static destructors.
     * In C it drives functions marked __attribute__((destructor)).
     * Same reason as .init_array — newlib generates entries here
     * even if nothing of yours uses it. The linker needs a home for them.
     */
    .fini_array : {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT(.fini_array.*)))
        KEEP (*(.fini_array*))
        PROVIDE_HIDDEN (__fini_array_end = .);
        . = ALIGN(4);
    } >FLASH

    _sidata = LOADADDR(.data);

    .data : {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } >RAM AT>FLASH

    .bss : {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } >RAM

    . = ALIGN(4);
    PROVIDE(end = .);
    PROVIDE(_end = .);
}