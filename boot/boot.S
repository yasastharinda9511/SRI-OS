.section ".text.boot"
.global _start

_start:
    // Only run on core 0
    mrc p15, 0, r0, c0, c0, 5
    and r0, r0, #3
    cmp r0, #0
    bne halt

    // Check if in HYP mode (0x1A)
    mrs r0, cpsr
    and r0, r0, #0x1F
    cmp r0, #0x1A
    bne not_hyp

    // Switch from HYP to SVC mode
    mrs r0, cpsr
    bic r0, r0, #0x1F
    orr r0, r0, #0x13       // SVC mode
    orr r0, r0, #0xC0       // Disable IRQ/FIQ
    msr spsr_hyp, r0
    ldr r0, =in_svc
    msr elr_hyp, r0
    eret

not_hyp:
in_svc:
    // Now in SVC mode - set stack
    ldr sp, =0x80000

    // Set VBAR
    ldr r0, =_vectors
    mcr p15, 0, r0, c12, c0, 0

    // Clear BSS
    ldr r0, =__bss_start
    ldr r1, =__bss_end
zero_bss:
    cmp r0, r1
    bhs bss_done
    mov r2, #0
    str r2, [r0], #4
    b zero_bss

bss_done:
    bl kernel_main

halt:
    wfe
    b halt