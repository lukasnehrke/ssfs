// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "connection.h"
#include "request.h"

static volatile int waiting;

static void die(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

static void sigint_handler(int signum) {
    (void) signum;
    waiting = 1;
}

static int listen_or_die(uint16_t port) {
    struct sockaddr_in6 name = {
        .sin6_family = AF_INET6,
        .sin6_port   = htons(port),
        .sin6_addr   = in6addr_any,
    };

    int sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock == -1) die("socket");

    int flag = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag))) die("setsockopt");

    if (bind(sock, (struct sockaddr *) &name, sizeof(name))) die("bind");
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
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    init_connection();
    if (init_request(argv[1])) die("init_request");

    /* register SIGINT handler */
    struct sigaction sa = { .sa_handler = &sigint_handler, .sa_flags = SA_RESTART };
    if (sigaction(SIGINT, &sa, NULL)) die("sigaction");

    /* ignore SIGPIPE */
    struct sigaction sa_pipe = { .sa_handler = SIG_IGN };
    if (sigaction(SIGPIPE, &sa_pipe, NULL)) die("sigaction");

    /* start worker thread */
    pthread_t tid;
    const int sock = listen_or_die(8080);
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

    if (close(sock)) perror("close");
    exit(EXIT_SUCCESS);
}
