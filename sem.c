// SPDX-License-Identifier: MIT

#include <errno.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct sem_s {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int value;
} sem_s;

sem_s *sem_create(int value) {
    sem_s *sem = (sem_s *) malloc(sizeof(sem_s));
    if (sem == NULL) return NULL;

    int err = pthread_mutex_init(&sem->mutex, NULL);
    if (err) {
        free(sem);
        errno = err;
        return NULL;
    }

    err = pthread_cond_init(&sem->cond, NULL);
    if (err) {
        pthread_mutex_destroy(&sem->mutex);
        free(sem);
        errno = err;
        return NULL;
    }

    sem->value = value;
    return sem;
}

void sem_destroy(sem_s *sem) {
    if (sem == NULL) return;
    pthread_cond_destroy(&sem->cond);
    pthread_mutex_destroy(&sem->mutex);
    free(sem);
}

void P(sem_s *sem) {
    pthread_mutex_lock(&sem->mutex);
    while (sem->value <= 0) pthread_cond_wait(&sem->cond, &sem->mutex);
    sem->value -= 1;
    pthread_mutex_unlock(&sem->mutex);
}

void V(sem_s *sem) {
    pthread_mutex_lock(&sem->mutex);
    sem->value += 1;
    pthread_cond_signal(&sem->cond);
    pthread_mutex_unlock(&sem->mutex);
}
