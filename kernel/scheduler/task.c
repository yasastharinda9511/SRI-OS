#include "task.h"
#include "../../drivers/uart/uart.h"
#include "../interrupts/interrupts.h"
#include <stddef.h>

static Task tasks[MAX_TASKS];
static int current_task_index = -1;
static int scheduler_running = 0;

// Pointer to current task's SP storage (for IRQ handler)
uint32_t** current_sp_ptr = NULL;

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
    current_sp_ptr = NULL;
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
     * Stack layout for preemptive scheduler:
     * Must match IRQ handler's restore order:
     *   ldmfd sp!, {r0}          <- SPSR
     *   ldmfd sp!, {r0-r12, lr, pc}^
     *
     * So stack (from low to high address):
     *   SPSR, r0, r1, r2, r3, r4-r12, lr, pc
     */
    uint32_t* sp = &task->stack[TASK_STACK_SIZE / 4];  // Start at top
    
    *(--sp) = (uint32_t)task_wrapper;  // pc
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
    *(--sp) = 0x13;                     // SPSR - SVC mode, IRQ enabled
    
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
    if (current_task_index < 0) {
        // No current task, find first ready
        for (int i = 0; i < MAX_TASKS; i++) {
            if (tasks[i].state == TASK_READY) {
                return i;
            }
        }
        return -1;
    }
    
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

// Called from IRQ handler - preemptive scheduling
uint32_t* preempt_schedule(uint32_t* current_sp) {
    if (!scheduler_running) {
        return 0;  // Return 0 means no switch
    }
    
    // Save current task's SP
    if (current_task_index >= 0) {
        tasks[current_task_index].stack_pointer = current_sp;
    }
    
    // Find next task
    int next = find_next_task();
    
    if (next < 0) {
        return 0;  // No task to switch to
    }
    
    if (next == current_task_index) {
        return 0;  // Same task, no switch needed
    }
    
    // Perform switch
    int prev = current_task_index;
    current_task_index = next;
    
    if (prev >= 0 && tasks[prev].state == TASK_RUNNING) {
        tasks[prev].state = TASK_READY;
    }
    
    tasks[next].state = TASK_RUNNING;
    current_sp_ptr = &tasks[next].stack_pointer;
    
    return tasks[next].stack_pointer;  // Return new SP
}

// Called from user code - cooperative scheduling
void schedule(void) {
    if (!scheduler_running) {
        return;
    }
    
    int next = find_next_task();
    
    if (next < 0 || next == current_task_index) {
        return;
    }
    
    int prev = current_task_index;
    current_task_index = next;
    
    if (prev >= 0 && tasks[prev].state == TASK_RUNNING) {
        tasks[prev].state = TASK_READY;
    }
    
    tasks[next].state = TASK_RUNNING;
    current_sp_ptr = &tasks[next].stack_pointer;
    
    if (prev >= 0) {
        context_switch(&tasks[prev].stack_pointer, tasks[next].stack_pointer);
    } else {
        context_switch(0, tasks[next].stack_pointer);
    }
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
    current_sp_ptr = &tasks[current_task_index].stack_pointer;
    
    uart_puts("Scheduler: Running task '");
    uart_puts(tasks[current_task_index].name);
    uart_puts("'\n\n");
    
    // Jump to first task
    context_switch(0, tasks[current_task_index].stack_pointer);
    
    uart_puts("Scheduler: ERROR - scheduler_start returned!\n");
    while(1);
}

void scheduler_tick(void) {
    // For preemptive: do nothing here, scheduling happens in IRQ handler
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
    
    uart_puts("Scheduler: All tasks terminated.\n");
    while (1) {
        __asm__ __volatile__("wfi");
    }
}

void task_yield(void) {
    schedule();
}

void task_sleep(uint32_t ticks) {
    if (current_task_index < 0) return;
    
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

Task* get_current_task(void) {
    if (current_task_index >= 0) {
        return &tasks[current_task_index];
    }
    return NULL;
}