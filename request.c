// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "connection.h"
#include "file.h"
#include "http.h"

static char *wwwPath;

int init_request(char *arg) {
    wwwPath = realpath(arg, NULL);
    if (wwwPath == NULL) return -1;

    return 0;
}

void handle_request(FILE *rx, FILE *tx) {
    char *method, *path, *protocol;

    errno = 0;
    if (parse_request(rx, &method, &path, &protocol)) {
        if (errno) {
            perror("parse_request");
            http_internal_server_error(tx);
            return;
        }
        http_bad_request(tx);
        return;
    }

    if (strcmp(method, "GET") != 0) {
        http_bad_request(tx);
        return;
    }

    if (strcmp(protocol, "HTTP/1.0") != 0 && strcmp(protocol, "HTTP/1.1") != 0) {
        http_bad_request(tx);
        return;
    }

    /* concat base path and request path */
    char *real_path = resolve_path(wwwPath, path);
    if (real_path == NULL) {
        if (errno == EACCES) {
            http_forbidden(tx);
            return;
        }
        http_internal_server_error(tx);
        return;
    }

    /* open requested file */
    FILE *file = open_file(real_path);
    if (file == NULL) {
        if (errno == EACCES) {
            http_forbidden(tx);
        } else if (errno == ENOENT) {
            http_not_found(tx);
        } else {
            perror(real_path);
            http_internal_server_error(tx);
        }
        free(real_path);
        return;
    }

    http_ok(tx);

    if (send_file(file, tx)) perror("send_file");
    if (fclose(file)) perror("fclose");
    free(real_path);
}
