#include "recursive_mutex.h"

#include <pthread.h>
#include <stdint.h>

void recursive_mutex_lock(recursive_mutex *mtx) {
    pthread_mutex_lock(&(mtx->lock));

    if (!pthread_equal(mtx->thread_using, pthread_self())) {
        while (mtx->recursive_count != 0) {
            pthread_cond_wait(&(mtx->cond), &(mtx->lock));
        }
        mtx->thread_using = pthread_self();
    }

    mtx->recursive_count++;

    pthread_mutex_unlock(&(mtx->lock));
}

void recursive_mutex_unlock(recursive_mutex *mtx) {
    pthread_mutex_lock(&(mtx->lock));

    mtx->recursive_count--;
    if (mtx->recursive_count == 0)
        pthread_cond_signal(&(mtx->cond));

    pthread_mutex_unlock(&(mtx->lock));
}

