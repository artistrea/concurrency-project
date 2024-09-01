#ifndef RECURSIVE_MUTEX_H
#define RECURSIVE_MUTEX_H

#include <pthread.h>
#include <stdint.h>

/*
// in POSIX systems it is also possible to make a recursive mutex using its pthread_mutexattr_t
pthread_mutex_t mutex;
pthread_mutexattr_t mutex_attr;

// run once when initializing mutex:
{
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mutex, &mutex_attr);
}
*/

typedef struct {
    uint32_t recursive_count;
    pthread_t thread_using;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} recursive_mutex;

#define RECURSIVE_MUTEX_INITIALIZER {0, 0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER}

void recursive_mutex_lock(recursive_mutex *mtx) ;

void recursive_mutex_unlock(recursive_mutex *mtx);

#endif // RECURSIVE_MUTEX_H
