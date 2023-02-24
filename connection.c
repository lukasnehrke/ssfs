// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include "rbuffer.h"
#include "request.h"
#include "connection.h"

#define POISON -2
#define BUFSIZE 16
#define NUM_THREADS 4

static pthread_t *thread_ids;
static rbuffer_s *rb;

static void die(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

static void *worker(void *arg) {
    (void) arg;
    while (1) {
        int client = rbuffer_get(rb);
        if (client == POISON) break;

        FILE *rx = fdopen(client, "r");
        if (rx == NULL) {
            perror("fdopen");
            close(client);
            continue;
        }

        int client_dup = dup(client);
        if (client_dup == -1) {
            perror("dup");
            fclose(rx);
            continue;
        }

        FILE *tx = fdopen(client_dup, "w");
        if (tx == NULL) {
            perror("fdopen");
            fclose(rx);
            close(client_dup);
            continue;
        }

        handle_request(rx, tx);

        if (fclose(rx)) perror("fclose");
        if (fclose(tx)) perror("fclose");
    }

    return NULL;
}

void init_connection(void) {
    rb = rbuffer_create(BUFSIZE);
    if (rb == NULL) die("rbuffer_create");

    thread_ids = (pthread_t *) calloc(NUM_THREADS, sizeof(pthread_t));
    if (thread_ids == NULL) die("calloc");

    /* start worker threads */
    for (int i = 0; i < NUM_THREADS; i++) {
        errno = pthread_create(&thread_ids[i], NULL, worker, NULL);
        if (errno) die("pthread_create");
    }
}

void shutdown_connection(void) {
    /* shutdown worker threads */
    for (int i = 0; i < NUM_THREADS; i++) {
        rbuffer_put(rb, POISON);
    }

    /* join worker threads */
    for (int i = 0; i < NUM_THREADS; i++) {
        errno = pthread_join(thread_ids[i], NULL);
        if (errno) perror("pthread_join");
    }

    rbuffer_destroy(rb);
    free(thread_ids);
}

void handle_client(int client) {
    if (client < 0) return;
    rbuffer_put(rb, client);
}
