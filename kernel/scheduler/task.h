#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#define MAX_TASKS       8
#define TASK_STACK_SIZE 4096
#define TASK_NAME_LEN   32

typedef enum {
    TASK_UNUSED = 0,
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_SLEEPING,
    TASK_TERMINATED
} TaskState;

typedef void (*TaskFunction)(void);

typedef struct {
    uint32_t id;
    char name[TASK_NAME_LEN];
    TaskState state;
    uint32_t* stack_pointer;
    uint32_t stack[TASK_STACK_SIZE / 4];
    uint32_t priority;
    uint32_t sleep_until;
} Task;

// Pointer to current task's SP storage (used by IRQ handler)
extern uint32_t** current_sp_ptr;

// Assembly function
extern void context_switch(uint32_t** old_sp, uint32_t* new_sp);

// Scheduler functions
void scheduler_init(void);
void scheduler_start(void);
void scheduler_tick(void);

// Preemptive scheduler function (called from IRQ)
uint32_t* preempt_schedule(uint32_t* current_sp);

// Task functions
int task_create(const char* name, TaskFunction func, uint32_t priority);
void task_exit(void);
void task_yield(void);
void task_sleep(uint32_t ticks);
void task_list(void);

// Task info
Task* task_current(void);
Task* get_current_task(void);
int task_count(void);

#endif