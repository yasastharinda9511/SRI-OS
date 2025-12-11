.section ".text.vectors"
.balign 32
.global _vectors

_vectors:
    ldr pc, reset_addr
    ldr pc, undef_addr
    ldr pc, swi_addr
    ldr pc, prefetch_addr
    ldr pc, data_addr
    ldr pc, unused_addr
    ldr pc, irq_addr
    ldr pc, fiq_addr

reset_addr:     .word _start
undef_addr:     .word hang
swi_addr:       .word hang
prefetch_addr:  .word hang
data_addr:      .word hang
unused_addr:    .word hang
irq_addr:       .word irq_handler_asm
fiq_addr:       .word hang

hang:
    b hang

.global irq_handler_asm
irq_handler_asm:
    // 1. Set stack FIRST
    ldr sp, =0x78000
    
    // 2. Save return address
    sub lr, lr, #4
    
    // 3. Save ALL registers BEFORE using any
    stmfd sp!, {r0-r12, lr}
    
    // 4. NOW safe to use registers - toggle GPIO for debug
    ldr r0, =0x3F20001C
    mov r1, #(1 << 23)
    str r1, [r0]
    
    // 5. Call C handler
    bl irq_handler_c
    
    // 6. Restore and return
    ldmfd sp!, {r0-r12, pc}^