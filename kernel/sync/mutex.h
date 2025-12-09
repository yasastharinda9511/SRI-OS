#ifndef MUTEX_H
#define MUTEX_H
#include <stdint.h>
#include <stdatomic.h>

typedef struct {
    atomic_int lock;
    uint32_t owner_task_id;
    const char* name;
} Mutex;

void mutex_init(Mutex* mtx, const char* name);
void mutex_lock(Mutex* mtx);
void mutex_unlock(Mutex* mtx);  
int mutex_is_locked(Mutex* mtx);
int mutex_try_lock(Mutex* mtx);

#endif