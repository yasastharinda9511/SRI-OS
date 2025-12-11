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
irq_addr:       .word irq_preempt
fiq_addr:       .word hang

hang:
    b hang

.extern irq_handler_c
.extern preempt_schedule
.extern current_sp_ptr

/*
 * Preemptive IRQ handler
 * 
 * Context saved on task stack:
 *   SPSR, r0-r12, lr, pc  (16 words)
 */
.global irq_preempt
irq_preempt:
    // Adjust return address
    sub lr, lr, #4
    
    // Use IRQ stack for temporary storage
    ldr sp, =0x78000
    
    // Save r0-r2 temporarily (we need them as work registers)
    stmfd sp!, {r0-r2}
    
    // r0 = SPSR, r1 = return address
    mrs r0, spsr
    mov r1, lr
    
    // Get SVC mode SP into r2
    // Switch to SVC mode briefly
    mrs r2, cpsr
    orr r2, r2, #0xC0         // Disable IRQ+FIQ
    bic r2, r2, #0x1F
    orr r2, r2, #0x13         // SVC mode
    msr cpsr_c, r2
    
    // Now in SVC mode
    mov r2, sp                // r2 = SVC stack pointer
    
    // Save context to SVC stack (task's stack)
    // Push PC (return address)
    stmfd r2!, {r1}
    // Push lr
    stmfd r2!, {lr}
    // Push r3-r12
    stmfd r2!, {r3-r12}
    
    // Get back r0-r2 from IRQ stack
    // Switch to IRQ mode
    mrs r3, cpsr
    bic r3, r3, #0x1F
    orr r3, r3, #0x12         // IRQ mode
    msr cpsr_c, r3
    
    // Pop saved r0-r2 into r3-r5
    ldmfd sp!, {r3-r5}        // r3=orig_r0, r4=orig_r1, r5=orig_r2
    
    // Back to SVC mode
    mrs r6, cpsr
    bic r6, r6, #0x1F
    orr r6, r6, #0x13         // SVC mode
    orr r6, r6, #0xC0         // IRQ+FIQ disabled
    msr cpsr_c, r6
    
    // Continue saving: push r0-r2 (from r3-r5)
    stmfd r2!, {r3-r5}
    // Push SPSR (still in r0)
    stmfd r2!, {r0}
    
    // r2 now points to saved context
    // Update actual SP
    mov sp, r2
    
    // Save SP for scheduler
    ldr r4, =current_sp_ptr
    ldr r4, [r4]
    cmp r4, #0
    strne sp, [r4]            // Save current SP if valid
    
    // Call IRQ handler (clears interrupt)
    bl irq_handler_c
    
    // Call scheduler - returns new SP
    mov r0, sp                // Current SP
    bl preempt_schedule       // Returns new SP or 0
    
    cmp r0, #0
    movne sp, r0              // Use new SP if switch happened
    
    // Restore context
    ldmfd sp!, {r0}           // SPSR
    msr spsr_cxsf, r0
    ldmfd sp!, {r0-r12, lr, pc}^   // Restore all and return