#include "task.h"
#include "../../drivers/uart/uart.h"
#include "../interrupts/interrupts.h"
#include <stddef.h>

static Task tasks[MAX_TASKS];
static int current_task_index = -1;
static int scheduler_running = 0;

static void str_copy(char* dst, const char* src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

void scheduler_init(void) {
    for (int i = 0; i < MAX_TASKS; i++) {
        tasks[i].state = TASK_UNUSED;
        tasks[i].id = i;
        tasks[i].stack_pointer = NULL;
        tasks[i].priority = 0;
        tasks[i].name[0] = '\0';
    }
    current_task_index = -1;
    scheduler_running = 0;
    uart_puts("Scheduler initialized.\n");
}

static void task_wrapper(TaskFunction func) {
    enable_irq();
    func();
    task_exit();
}

int task_create(const char* name, TaskFunction func, uint32_t priority) {
    
    int slot = -1;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_UNUSED) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        uart_puts("Scheduler: No free task slots!\n");
        return -1;
    }
    
    Task* task = &tasks[slot];
    task->priority = priority;
    str_copy(task->name, name, TASK_NAME_LEN);
    
    /*
     * Stack layout - MUST MATCH context.S pop order!
     * 
     * context.S does:
     *   pop {r0-r12}   ← 13 registers
     *   pop {lr}       ← 1 register
     *   pop {pc}       ← 1 register (jumps here)
     *                  ─────────────
     *                  Total: 15 values
     *
     * So we create stack (top to bottom, sp points to r0):
     *   [high address]
     *   pc  = task_wrapper  ← popped last, execution starts here
     *   lr  = 0
     *   r12 = 0
     *   r11 = 0
     *   r10 = 0
     *   r9  = 0
     *   r8  = 0
     *   r7  = 0
     *   r6  = 0
     *   r5  = 0
     *   r4  = 0
     *   r3  = 0
     *   r2  = 0
     *   r1  = 0
     *   r0  = func      ← argument to task_wrapper
     *   [low address]   ← sp points here
     */

    uint32_t* sp = &task->stack[TASK_STACK_SIZE / 4];  // Start at top
    
    *(--sp) = (uint32_t)task_wrapper;  // pc - where execution starts
    *(--sp) = 0;                        // lr
    *(--sp) = 0;                        // r12
    *(--sp) = 0;                        // r11
    *(--sp) = 0;                        // r10
    *(--sp) = 0;                        // r9
    *(--sp) = 0;                        // r8
    *(--sp) = 0;                        // r7
    *(--sp) = 0;                        // r6
    *(--sp) = 0;                        // r5
    *(--sp) = 0;                        // r4
    *(--sp) = 0;                        // r3
    *(--sp) = 0;                        // r2
    *(--sp) = 0;                        // r1
    *(--sp) = (uint32_t)func;           // r0 - argument to task_wrapper
    
    task->stack_pointer = sp;
    task->state = TASK_READY;
    task->sleep_until = 0;
    
    uart_puts("Scheduler: Created task '");
    uart_puts(name);
    uart_puts("' (ID ");
    uart_putc('0' + slot);
    uart_puts(")\n");
    
    return slot;
}

static int find_next_task(void) {
    int start = (current_task_index + 1) % MAX_TASKS;
    int idx = start;    

    do {
        // Check for sleeping tasks that should wake up
        if (tasks[idx].state == TASK_SLEEPING) {
            if (timer_ticks >= tasks[idx].sleep_until) {
                tasks[idx].state = TASK_READY;
            }
        }
        
        if (tasks[idx].state == TASK_READY) {
            return idx;
        }
        idx = (idx + 1) % MAX_TASKS;
    } while (idx != start);
    
    return -1;
}

void schedule(void) {
    if (!scheduler_running) {
        return;
    }
    
    int next = find_next_task();

    if (next < 0) {
        return;
    }

    if (next == current_task_index) {
        return;
    }

    int prev = current_task_index;
    current_task_index = next;

    if (prev >= 0 && tasks[prev].state == TASK_RUNNING) {
        tasks[prev].state = TASK_READY;
    }

    tasks[next].state = TASK_RUNNING;
    
    if (prev >= 0) {
        context_switch(&tasks[prev].stack_pointer, tasks[next].stack_pointer);
    } else {
        context_switch(0, tasks[next].stack_pointer);
    }
    
    // NOTE: When we return here, it means another task switched back to us.
    // This is NORMAL, not an error!
}

void scheduler_start(void) {
    uart_puts("Scheduler: Starting...\n");
    
    current_task_index = find_next_task();
    
    if (current_task_index < 0) {
        uart_puts("Scheduler: No tasks to run!\n");
        return;
    }
    
    scheduler_running = 1;
    tasks[current_task_index].state = TASK_RUNNING;
    
    uart_puts("Scheduler: Running task '");
    uart_puts(tasks[current_task_index].name);
    uart_puts("'\n\n");
    
    // Jump to first task - this "returns" to task_wrapper
    context_switch(0, tasks[current_task_index].stack_pointer);
    
    // Should never reach here because first task never returns to us
    uart_puts("Scheduler: ERROR - scheduler_start returned!\n");
    while(1);
}

void scheduler_tick(void) {
    schedule();
}

void task_exit(void) {
    if (current_task_index < 0) {
        return;
    }
    
    uart_puts("\nTask '");
    uart_puts(tasks[current_task_index].name);
    uart_puts("' exited.\n");
    
    tasks[current_task_index].state = TASK_TERMINATED;
    
    schedule();
    
    // If schedule returns, no more tasks
    uart_puts("Scheduler: All tasks terminated.\n");
    while (1) {
        __asm__ __volatile__("wfi");
    }
}

void task_yield(void) {
    schedule();
}

void task_sleep(uint32_t ms) {
    if (current_task_index < 0) return;
    
    uint32_t ticks = (ms + 999) / 1000;
    
    tasks[current_task_index].sleep_until = timer_ticks + ticks;
    tasks[current_task_index].state = TASK_SLEEPING;
    
    schedule();
}

Task* task_current(void) {
    if (current_task_index >= 0) {
        return &tasks[current_task_index];
    }
    return 0;
}

int task_count(void) {
    int count = 0;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state != TASK_UNUSED && 
            tasks[i].state != TASK_TERMINATED) {
            count++;
        }
    }
    return count;
}

void task_list(void) {
    uart_puts("\n");
    uart_puts("  ID  Name            State\n");
    uart_puts("  --  ----            -----\n");
    
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_UNUSED) continue;
        
        uart_puts("  ");
        uart_putc('0' + i);
        uart_puts("   ");
        
        uart_puts(tasks[i].name);
        int len = 0;
        const char* p = tasks[i].name;
        while (*p++) len++;
        int pad = 16 - len;
        while (pad-- > 0) uart_putc(' ');
        
        switch (tasks[i].state) {
            case TASK_READY:      uart_puts("Ready"); break;
            case TASK_RUNNING:    uart_puts("Running *"); break;
            case TASK_BLOCKED:    uart_puts("Blocked"); break;
            case TASK_SLEEPING:   uart_puts("Sleeping"); break;
            case TASK_TERMINATED: uart_puts("Terminated"); break;
            default:              uart_puts("Unknown"); break;
        }
        
        uart_puts("\n");
    }
    uart_puts("\n");
}