.section ".text.vectors"
    .global _vectors

_vectors:
    ldr pc, _reset_addr       // Reset
    ldr pc, _undef_addr       // Undefined Instruction
    ldr pc, _swi_addr         // Software Interrupt
    ldr pc, _prefetch_addr    // Prefetch Abort
    ldr pc, _data_addr        // Data Abort
    ldr pc, _reserved_addr    // Reserved
    ldr pc, _irq_addr         // IRQ
    ldr pc, _fiq_addr         // FIQ

_reset_addr:    .word _start
_undef_addr:    .word undefined_handler
_swi_addr:      .word swi_handler
_prefetch_addr: .word prefetch_handler
_data_addr:     .word data_handler
_reserved_addr: .word reserved_handler
_irq_addr:      .word irq_handler_asm    // <-- FIXED: was irq_handler
_fiq_addr:      .word fiq_handler

// Default handlers (infinite loop)
undefined_handler:
    b undefined_handler

swi_handler:
    b swi_handler

prefetch_handler:
    b prefetch_handler

data_handler:
    b data_handler

reserved_handler:
    b reserved_handler

fiq_handler:
    b fiq_handler

// IRQ Handler - saves context and calls C handler
    .global irq_handler_asm
irq_handler_asm:
    sub lr, lr, #4              // Adjust return address
    stmfd sp!, {r0-r12, lr}     // Save registers
    bl irq_handler_c            // Call C handler
    ldmfd sp!, {r0-r12, pc}^    // Restore and return (^ restores CPSR)