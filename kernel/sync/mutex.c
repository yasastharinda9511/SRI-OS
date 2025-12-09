
#include "mutex.h"
#include "../scheduler/task.h"


void mutex_init(Mutex* mtx, const char* name) {
    atomic_store(&mtx->lock, 0);
    mtx->owner_task_id = 0;
    mtx->name = name;
}


void mutex_lock(Mutex* mtx) {
    uint32_t current_task_id = get_current_task()->id;
    
    // Spin until we acquire the lock
    while (1) {
        int expected = 0;
        if (atomic_compare_exchange_strong(&mtx->lock, &expected, 1)) {
            // Acquired the lock
            mtx->owner_task_id = current_task_id;
            return;
        }
        
        // Optional: Yield to allow other tasks to run
        task_yield();
    }
}

int mutex_try_lock(Mutex* mtx) {
    uint32_t current_task_id = get_current_task()->id;
    
    int expected = 0;
    if (atomic_compare_exchange_strong(&mtx->lock, &expected, 1)) {
        // Acquired the lock
        mtx->owner_task_id = current_task_id;
        return 1;  // Success
    }
    
    return 0;  // Failed to acquire
}    

void mutex_unlock(Mutex* mtx) {
    uint32_t current_task_id = get_current_task()->id;
    
    // Only the owner can unlock
    if (mtx->owner_task_id != current_task_id) {
        return;
    }
    
    mtx->owner_task_id = 0;
    atomic_store(&mtx->lock, 0);
}

int mutex_is_locked(Mutex* mtx) {
    return atomic_load(&mtx->lock) != 0;
}