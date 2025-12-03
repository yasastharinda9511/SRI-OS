ENTRY(_start)

SECTIONS
{
    /* Kernel loads at 0x10000 (64KB) - below this is vector table and stacks */
    . = 0x10000;

    __start = .;

    .text : {
        KEEP(*(.text.boot))      /* Boot code first */
        KEEP(*(.text.vectors))   /* Vector table */
        *(.text*)                /* All other code */
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    }

    __end = .;
}