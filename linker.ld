SECTIONS
{
    /* Put vector table at 0x00000000 */
    . = 0x0000;

    .vectors : {
        KEEP(*(.text.vectors))
    }

    /* Raspberry Pi loads kernel at 0x8000 */
    . = 0x8000;

    __start = .;

    .text : {
        KEEP(*(.text.boot))
        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(16);
        __bss_end = .;
    }

    . = ALIGN(16);
    __end = .;

    __heap_start = .;
}
