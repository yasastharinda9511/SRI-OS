/*-----------------------------------------------------------------------*/
/* Context Switch Assembly for ARM                                       */
/*-----------------------------------------------------------------------*/

    .global context_switch
    .type context_switch, %function

/*
 * void context_switch(uint32_t** old_sp, uint32_t* new_sp)
 *
 * r0 = pointer to old task's sp storage (NULL if no old task)
 * r1 = new task's sp value
 *
 * Stack frame (15 registers):
 *   r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, lr, pc
 */

context_switch:
    /* Check if we need to save old context */
    cmp r0, #0
    beq load_new_context

    /* Save old task's context */
    push {lr}                   /* Save return address (will be PC) */
    push {lr}                   /* Save LR */
    push {r0-r12}               /* Save r0-r12 */

    /* Save stack pointer to old task's TCB */
    str sp, [r0]

load_new_context:
    /* Load new task's stack pointer */
    mov sp, r1

    /* Restore new task's context */
    pop {r0-r12}                /* Restore r0-r12 */
    pop {lr}                    /* Restore LR */
    pop {pc}                    /* Jump to saved PC */

    .size context_switch, . - context_switch