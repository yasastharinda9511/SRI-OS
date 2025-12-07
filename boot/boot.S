    .section ".text.boot"
    .global _start

    // CPU Modes
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

    // -----------------------------
    // Setup IRQ stack
    // -----------------------------
    mov r0, #(MODE_IRQ | 0x80 | 0x40)   // IRQ mode, IRQ/FIQ disabled
    msr cpsr_c, r0
    ldr sp, =STACK_IRQ

    // -----------------------------
    // Setup FIQ stack
    // -----------------------------
    mov r0, #(MODE_FIQ | 0x80 | 0x40)   // FIQ mode, IRQ/FIQ disabled
    msr cpsr_c, r0
    ldr sp, =STACK_FIQ

    // -----------------------------
    // Setup SVC stack
    // -----------------------------
    mov r0, #(MODE_SVC | 0x80 | 0x40)   // SVC mode, IRQ/FIQ disabled
    msr cpsr_c, r0
    ldr sp, =STACK_SVC

    // -----------------------------
    // Zero out BSS
    // -----------------------------
    ldr r0, =__bss_start
    ldr r1, =__bss_end

zero_loop:
    cmp r0, r1
    bhs zero_done
    mov r2, #0
    str r2, [r0], #4
    b zero_loop

zero_done:

    // -----------------------------
    // Jump to C kernel main
    // -----------------------------
    bl kernel_main

hang:
    wfi         // Wait for interrupt
    b hang
