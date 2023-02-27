// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include "args.h"
#include "rbuffer.h"
#include "request.h"
#include "connection.h"

#define POISON -2

static int threads;
static pthread_t *thread_ids;
static rbuffer_s *rb;

static void die(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

static int parse_flag(const char *flag, int def) {
    const char *value = get_flag(flag);
    if (value == NULL) return def;

    char *endptr;
    errno = 0;
    long x = strtol(value, &endptr, 10);
    if (errno != 0) die("strtol");

    if (value == endptr || *endptr != '\0') {
        fprintf(stderr, "%s is an invalid number\n", flag);
        exit(EXIT_FAILURE);
    }

    if (x <= 0) {
        fprintf(stderr, "%s must be greater 0\n", flag);
        exit(EXIT_FAILURE);
    }

    if (x >= INT_MAX) {
        fprintf(stderr, "%s is too large\n", flag);
        exit(EXIT_FAILURE);
    }

    return (int) x;
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

int init_connection(void) {
    /* parse args */
    int bufsize = parse_flag("bufsize", 16);
    threads = parse_flag("threads", 4);

    rb = rbuffer_create(bufsize);
    if (rb == NULL) die("rbuffer_create");

    thread_ids = (pthread_t *) calloc(threads, sizeof(pthread_t));
    if (thread_ids == NULL) die("calloc");

    /* start worker threads */
    for (int i = 0; i < threads; i++) {
        errno = pthread_create(&thread_ids[i], NULL, worker, NULL);
        if (errno) die("pthread_create");
    }

    return 0;
}

void shutdown_connection(void) {
    /* shutdown worker threads */
    for (int i = 0; i < threads; i++) {
        rbuffer_put(rb, POISON);
    }

    /* join worker threads */
    for (int i = 0; i < threads; i++) {
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
