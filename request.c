// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "request.h"
#include "args.h"
#include "file.h"
#include "http.h"

static char *wwwPath;

static void die(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

int init_request(void) {
    const char *path = get_flag("path");
    if (path == NULL) return -1;

    wwwPath = realpath(path, NULL);
    if (wwwPath == NULL) die(path);

    return 0;
}

void shutdown_request(void) {
    free(wwwPath);
}

void handle_request(FILE *rx, FILE *tx) {
    /* read status line */
    char request[8193];
    if (fgets(request, sizeof(request), rx) == NULL) {
        if (ferror(rx)) {
            perror("fgets");
            http_internal_server_error(tx);
            return;
        }
        http_bad_request(tx);
        return;
    }

    /* ensure newline is present */
    size_t len = strlen(request);
    if (len == 0 || request[len - 1] != '\n') {
        http_bad_request(tx);
        return;
    }

    if (len > 1 && request[len - 2] == '\r') {
        request[len - 2] = '\0';
    } else {
        request[len - 1] = '\0';
    }

    /* parse status line */
    char *rest;
    char *method   = strtok_r(request, " ", &rest);
    char *path     = strtok_r(NULL, " ", &rest);
    char *protocol = strtok_r(NULL, "", &rest);

    if (method == NULL || path == NULL || protocol == NULL) {
        http_bad_request(tx);
        return;
    }

    /* only GET is allowed */
    if (strcmp(method, "GET") != 0) {
        http_bad_request(tx);
        return;
    }

    /* check for valid protocol */
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
        perror("resolve_path");
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
