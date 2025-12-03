    .section ".text.boot"
    .global _start


    // Define CPU modes
    .equ MODE_USR, 0x10
    .equ MODE_FIQ, 0x11
    .equ MODE_IRQ, 0x12
    .equ MODE_SVC, 0x13
    .equ MODE_ABT, 0x17
    .equ MODE_UND, 0x1B
    .equ MODE_SYS, 0x1F

    // Stack Locations
    .equ STACK_IRQ, 0x8000
    .equ STACK_FIQ, 0x7000
    .equ STACK_SVC, 0x90000


_start:
    
    // Enter IRQ mode and set Stack
    mov r0, #(MODE_IRQ | 0x80| 0x40) // IRQ mode with IRQ and FIQ disabled
    msr cpsr_c, r0
    ldr sp, =STACK_IRQ

    // Enter FIQ mode and set Stack
    mov r0, #(MODE_FIQ | 0x80| 0x40) // FIQ mode with IRQ and FIQ disabled
    msr cpsr_c, r0
    ldr sp, =STACK_FIQ

    // Enter SVC mode and set Stack
    mov r0, #(MODE_SVC | 0x80| 0x40) // SVC mode with IRQ and FIQ disabled
    msr cpsr_c, r0
    ldr sp, =STACK_SVC

    // Zero out the BSS section
    ldr r0, =__bss_start
    ldr r1, =__bss_end

zero_loop:
    cmp r0, r1
    bhs zero_done
    mov r2, #0
    str r2, [r0], #4
    b zero_loop

zero_done:
    bl kernel_main

hang:
    // If kernel_main returns, halt
    wfi         // Wait for interrupt (low power)
    b hang