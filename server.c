// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "connection.h"

static void die(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

int main(void) {
    init_connection();

    struct sockaddr_in6 name = {
        .sin6_family = AF_INET6,
        .sin6_port   = htons(8080),
        .sin6_addr   = in6addr_any,
    };

    int sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock == -1) die("socket");

    int flag = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag))) die("setsockopt");

    if (bind(sock, (struct sockaddr *) &name, sizeof(name))) die("bind");
    if (listen(sock, SOMAXCONN)) die("listen");

    while (1) {
        int client = accept(sock, NULL, NULL);
        if (client == -1) {
            perror("accept");
            continue;
        }

        handle_client(client);
    }
}
