.global context_switch
.type context_switch, %function

/*
 * void context_switch(uint32_t** old_sp, uint32_t* new_sp)
 *
 * Stack layout (matches IRQ handler):
 *   [sp+0]  = SPSR
 *   [sp+4]  = r0
 *   [sp+8]  = r1
 *   ...
 *   [sp+56] = lr
 *   [sp+60] = pc
 */

context_switch:
    cmp r0, #0
    beq 1f

    /* Save context to stack */
    sub sp, sp, #64           /* Allocate 16 words */
    
    str lr, [sp, #60]         /* pc = return address */
    str lr, [sp, #56]         /* lr */
    str r12, [sp, #52]
    str r11, [sp, #48]
    str r10, [sp, #44]
    str r9, [sp, #40]
    str r8, [sp, #36]
    str r7, [sp, #32]
    str r6, [sp, #28]
    str r5, [sp, #24]
    str r4, [sp, #20]
    str r3, [sp, #16]
    str r2, [sp, #12]
    str r1, [sp, #8]
    str r0, [sp, #4]
    
    mrs r2, cpsr
    str r2, [sp, #0]          /* SPSR = current CPSR */
    
    /* Save SP to old_sp pointer */
    str sp, [r0]

1:
    /* Load new SP */
    mov sp, r1
    
    /* Restore SPSR */
    ldr r0, [sp, #0]
    msr spsr_cxsf, r0
    
    /* Restore registers */
    ldr r0, [sp, #4]
    ldr r1, [sp, #8]
    ldr r2, [sp, #12]
    ldr r3, [sp, #16]
    ldr r4, [sp, #20]
    ldr r5, [sp, #24]
    ldr r6, [sp, #28]
    ldr r7, [sp, #32]
    ldr r8, [sp, #36]
    ldr r9, [sp, #40]
    ldr r10, [sp, #44]
    ldr r11, [sp, #48]
    ldr r12, [sp, #52]
    ldr lr, [sp, #56]
    
    /* Load PC and restore CPSR from SPSR */
    add sp, sp, #60
    ldmfd sp!, {pc}^

.size context_switch, . - context_switch