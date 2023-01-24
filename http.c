// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "http.h"

int parse_request(FILE *rx, char **method, char **path, char **protocol) {
    int err = errno;

    /* read status line */
    char request[8193];
    if (fgets(request, sizeof(request), rx) == NULL) {
        if (ferror(rx)) return -1;
        errno = err;
        return -1;
    }

    /* ensure newline is present */
    size_t len = strlen(request);
    if (len == 0 || request[len - 1] != '\n') {
        err = errno;
        return -1;
    }

    if (len > 1 && request[len - 2] == '\r') {
        request[len - 2] = '\0';
    } else {
        request[len - 1] = '\0';
    }

    /* parse status line */
    char *rest;
    *method   = strtok_r(request, " ", &rest);
    *path     = strtok_r(NULL, " ", &rest);
    *protocol = strtok_r(NULL, "", &rest);

    if (*method == NULL || *path == NULL || *protocol == NULL) {
        errno = err;
        return -1;
    }

    return 0;
}

void http_ok(FILE *tx) {
    fprintf(tx, "HTTP/1.1 200 OK\r\n");
    fprintf(tx, "Connection: close\r\n");
    fprintf(tx, "\r\n");
}

void http_bad_request(FILE *tx) {
    fprintf(tx, "HTTP/1.1 400 Bad Request\r\n");
    fprintf(tx, "Connection: close\r\n");
}

void http_forbidden(FILE *tx) {
    fprintf(tx, "HTTP/1.1 403 Forbidden\r\n");
    fprintf(tx, "Connection: close\r\n");
}

void http_not_found(FILE *tx) {
    fprintf(tx, "HTTP/1.1 404 Not Found\r\n");
    fprintf(tx, "Connection: close\r\n");
}

void http_internal_server_error(FILE *tx) {
    fprintf(tx, "HTTP/1.1 500 Internal Server Error\r\n");
    fprintf(tx, "Connection: close\r\n");
}
