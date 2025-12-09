#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <stdint.h>
#include <stdatomic.h>


typedef struct {
    atomic_int count;
    int max_count;
    const char* name;
} Semaphore;

void sem_init(Semaphore* s, const char* name, int initial, int max);
void sem_wait(Semaphore* s);
int sem_trywait(Semaphore* s);
void sem_signal(Semaphore* s);
int sem_getcount(Semaphore* s);

#endif