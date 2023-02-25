// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "http.h"

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
