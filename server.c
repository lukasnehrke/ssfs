// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "connection.h"
#include "request.h"

static void die(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
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

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    init_connection();
    if (init_request(argv[1])) die("init_request");

    struct sigaction sa = { .sa_handler = SIG_IGN };
    if (sigaction(SIGPIPE, &sa, NULL)) die("sigaction");

    int sock = listen_or_die(8080);

    while (1) {
        int client = accept(sock, NULL, NULL);
        if (client == -1) {
            perror("accept");
            continue;
        }

        handle_client(client);
    }
}
