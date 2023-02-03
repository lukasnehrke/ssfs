// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include "rbuffer.h"
#include "request.h"
#include "connection.h"

static rbuffer_s *rb;

static void die(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

static void *worker(void *arg) {
    (void) arg;
    while (1) {
        int client = rbuffer_get(rb);

        int client_dup = dup(client);
        if (client_dup == -1) {
            perror("dup");
            close(client);
            continue;
        }

        FILE *rx = fdopen(client, "r");
        if (rx == NULL) {
            perror("fdopen");
            close(client);
            close(client_dup);
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
    rb = rbuffer_create(16);
    if (rb == NULL) die("rbuffer_create");

    pthread_t thread;
    for (int i = 0; i < 4; i++) {
        errno = pthread_create(&thread, NULL, worker, NULL);
        if (errno) die("pthread_create");
    }
}

void handle_client(int client) {
    rbuffer_put(rb, client);
}
