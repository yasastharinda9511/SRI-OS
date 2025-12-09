#ifndef SPIN_LOCK_H
#define SPIN_LOCK_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

typedef atomic_flag Spinlock;

#define SPINLOCK_INIT ATOMIC_FLAG_INIT

void spin_init(Spinlock* lock);
void spin_lock(Spinlock* lock);
int spin_trylock(Spinlock* lock);
void spin_unlock(Spinlock* lock);


#endif