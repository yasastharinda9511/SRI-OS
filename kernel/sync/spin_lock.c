#include "spin_lock.h"

void spin_init(Spinlock* lock) {
    atomic_flag_clear(lock);
}

void spin_lock(Spinlock* lock) {
    while (atomic_flag_test_and_set(lock)) {
        // Busy-wait (spin)
    }
}

int spin_trylock(Spinlock* lock) {
    return !atomic_flag_test_and_set(lock);
}

void spin_unlock(Spinlock* lock) {
    atomic_flag_clear(lock);
}