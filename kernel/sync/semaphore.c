#include "semaphore.h"
#include "../scheduler/task.h"

void sem_init(Semaphore* s, const char* name, int initial, int max) {
    atomic_store(&s->count, initial);
    s->max_count = max;
    s->name = name;
}

void sem_wait(Semaphore* s) {
    while (1) {
        int current_count = atomic_load(&s->count);
        if (current_count > 0) {
            if (atomic_compare_exchange_strong(&s->count, &current_count, current_count - 1)) {
                return;
            }
        } else {
            // Optional: Yield to allow other tasks to run
            task_yield();
        }
    }
}

int sem_trywait(Semaphore* s) {
    int current_count = atomic_load(&s->count);
    if (current_count > 0) {
        if (atomic_compare_exchange_strong(&s->count, &current_count, current_count - 1)) {
            return 1;  // Success
        }
    }
    return 0;  // Failed to acquire
}

void sem_signal(Semaphore* s) {
    while (1) {
        int current_count = atomic_load(&s->count);
        if (current_count < s->max_count) {
            if (atomic_compare_exchange_strong(&s->count, &current_count, current_count + 1)) {
                return;
            }
        } else {
            return;  // Semaphore is already at max count
        }
    }
}

int sem_getcount(Semaphore* s) {
    return atomic_load(&s->count);
}