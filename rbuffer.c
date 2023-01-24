// SPDX-License-Identifier: MIT

#include <stdlib.h>
#include <stdatomic.h>
#include <errno.h>
#include "rbuffer.h"
#include "sem.h"

typedef struct rbuffer_s {
    atomic_int start;
    int end;
    int size;
    sem_s *sem_full;
    sem_s *sem_free;
    int buffer[];
} rbuffer_s;

rbuffer_s *rbuffer_create(int size) {
    if (size <= 0) {
        errno = EINVAL;
        return NULL;
    }

    rbuffer_s *rb = (rbuffer_s *) malloc(sizeof(rbuffer_s) + size * sizeof(int));
    if (rb == NULL) return NULL;

    rb->sem_free = sem_create(size);
    if (rb->sem_free == NULL) {
        int err = errno;
        free(rb);
        errno = err;
        return NULL;
    }

    rb->sem_full = sem_create(0);
    if (rb->sem_full == NULL) {
        int err = errno;
        sem_destroy(rb->sem_free);
        free(rb);
        errno = err;
        return NULL;
    }

    rb->start = ATOMIC_VAR_INIT(0);
    rb->end = 0;
    rb->size = size;

    return rb;
}

void rbuffer_destroy(rbuffer_s *rb) {
    if (rb == NULL) return;
    sem_destroy(rb->sem_free);
    sem_destroy(rb->sem_full);
    free(rb);
}

void rbuffer_put(rbuffer_s *rb, int value) {
    P(rb->sem_free);
    rb->buffer[rb->end++] = value;
    rb->end %= rb->size;
    V(rb->sem_full);
}

int rbuffer_get(rbuffer_s *rb) {
    P(rb->sem_full);

    int start, next, value;
    do {
        start = atomic_load(&rb->start);
        value = rb->buffer[start];
        next = (start + 1) % rb->size;
    } while (atomic_compare_exchange_strong(&rb->start, &start, next) == 0);

    V(rb->sem_free);
    return value;
}
