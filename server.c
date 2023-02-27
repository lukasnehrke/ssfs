// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "args.h"
#include "connection.h"
#include "request.h"

#define USAGE "%s --path=<path> [--port=<port>] [--threads=<amount>] [--bufsize=<size>]\n"

static volatile int waiting;

static void die(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

static void sigint_handler(int signum) {
    (void) signum;
    waiting = 1;
}

static uint16_t parse_port(const char *flag) {
    char *endptr;

    errno = 0;
    long x = strtol(flag, &endptr, 10);
    if (errno != 0) die("port is an invalid number");

    if (flag == endptr || *endptr != '\0') {
        fprintf(stderr, "port is an invalid number\n");
        exit(EXIT_FAILURE);
    }

    if (x < 0) {
        fprintf(stderr, "port must be positive\n");
        exit(EXIT_FAILURE);
    }

    if (x >= USHRT_MAX) {
        fprintf(stderr, "port is too large\n");
        exit(EXIT_FAILURE);
    }

    return (uint16_t) x;
}

static int listen_or_die(void) {
    uint16_t port = 8080;

    const char *port_flag = get_flag("port");
    if (port_flag != NULL) {
        port = parse_port(port_flag);
    }

    int sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock == -1) die("socket");

    int flag = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag))) die("setsockopt");

    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port	 = htons(port);
    addr.sin6_addr   = in6addr_any;

    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr))) die("bind");
    if (listen(sock, SOMAXCONN)) die("listen");

    return sock;
}

static void *worker(void *arg) {
    int sock = *((int *) arg);
    while (1) {
        int client = accept(sock, NULL, NULL);
        if (waiting) break;

        if (client == -1) {
            perror("accept");
            continue;
        }

        handle_client(client);
    }

    return NULL;
}

int main(int argc, char **argv) {
    if (parse_args(argc, argv)) die("parse_args");

    /* init modules */
    if (init_request() || init_connection()) {
        fprintf(stderr, USAGE, argv[0]);
        exit(EXIT_FAILURE);
    }

    const int sock = listen_or_die();

    /* register SIGINT handler */
    struct sigaction sa = { .sa_handler = &sigint_handler, .sa_flags = SA_RESTART };
    if (sigaction(SIGINT, &sa, NULL)) die("sigaction");

    /* ignore SIGPIPE */
    struct sigaction sa_pipe = { .sa_handler = SIG_IGN };
    if (sigaction(SIGPIPE, &sa_pipe, NULL)) die("sigaction");

    /* start worker thread */
    pthread_t tid;
    errno = pthread_create(&tid, NULL, worker, (void *) &sock);
    if (errno) die("pthread_create");

    /* wait for SIGINT */
    sigset_t old_mask, new_mask;
    if (sigemptyset(&new_mask)) die("sigemptyset");
    if (sigaddset(&new_mask, SIGINT)) die("sigaddset");
    if (sigprocmask(SIG_BLOCK, &new_mask, &old_mask)) die("sigprocmask");
    while (waiting == 0) sigsuspend(&old_mask);

    /* shutdown worker thread */
    if (shutdown(sock, SHUT_RDWR) == 0) {
        errno = pthread_join(tid, NULL);
        if (errno) perror("pthread_join");
    } else {
        perror("shutdown");
    }

    shutdown_connection();
    shutdown_request();

    if (close(sock)) perror("close");
    exit(EXIT_SUCCESS);
}
