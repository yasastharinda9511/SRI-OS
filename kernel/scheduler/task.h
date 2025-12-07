#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#define MAX_TASKS 32
#define TASK_NAME_LEN 32
#define TASK_STACK_SIZE 4096  // 4 KB stack per task


typedef enum {
    TASK_UNUSED,
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_TERMINATED,
    TASK_SLEEPING
} TaskState;


typedef struct {
    uint32_t *stack_pointer;
    uint32_t id;
    char name[TASK_NAME_LEN];
    TaskState state;
    uint32_t priority;
    uint32_t stack[TASK_STACK_SIZE / 4]; // Stack memory
    uint32_t sleep_until;              // For sleeping tasks
} Task;


typedef void (*TaskFunction)(void);

void scheduler_init(void);
void scheduler_start(void);
void schedule(void);

int task_create(const char* name, TaskFunction func, uint32_t priority);
void task_yield(void);
void task_exit(void);   
void task_sleep(uint32_t ticks);

Task* get_current_task(void);
int task_count(void);
void task_list(void);

void scheduler_tick(void);

extern void context_switch(uint32_t** old_sp, uint32_t* new_sp);

#endif