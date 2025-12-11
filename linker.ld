ENTRY(_start)

SECTIONS
{
    /* Kernel loads at 0x8000 */
    . = 0x8000;
    __start = .;

    .text.boot : {
        KEEP(*(.text.boot))
    }

    .text.vectors : ALIGN(32) {
        KEEP(*(.text.vectors))
    }

    .text : {
        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : ALIGN(16) {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(16);
        __bss_end = .;
    }

    __end = .;
    __heap_start = .;
}