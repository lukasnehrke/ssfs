// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include "request.h"
#include "args.h"
#include "http.h"

static void die(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

static int check_index(FILE *tx, int base) {
    int fd = openat(base, "index.html", O_RDONLY);
    if (fd == -1) {
        if (errno == ENOENT || errno == EACCES) return 0;
        perror("index.html");
        return -1;
    }

    struct stat st;
    if (fstat(fd, &st)) {
        perror("fstat");
        close(fd);
        return -1;
    }

    if (!S_ISREG(st.st_mode)) {
        close(fd);
        return 0;
    }

    http_sendfile(fd, tx);
    return 1;
}

int init_request(void) {
    const char *path = get_flag("path");
    if (path == NULL) return -1;

    int base = open(path, O_RDONLY | O_DIRECTORY);
    if (base == -1) die(path);
    if (close(base)) perror("close");

    return 0;
}

void shutdown_request(void) {}

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

    /* check for valid method */
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
    const char *base = get_flag("path");
    char full_path[strlen(base) + strlen(path) + 2];
    if (snprintf(full_path, sizeof(full_path), "%s/%s", base, path) < 0) {
        perror("snprintf");
        http_internal_server_error(tx);
        return;
    }

    int fd = open(full_path, O_RDONLY);
    if (fd == -1) {
        if (errno == ENOENT) {
            http_not_found(tx);
            return;
        }
        if (errno == EACCES) {
            http_forbidden(tx);
            return;
        }
        perror(full_path);
        http_internal_server_error(tx);
        return;
    }

    struct stat st;
    if (fstat(fd, &st)) {
        perror("fstat");
        http_internal_server_error(tx);
        close(fd);
        return;
    }

    if (S_ISREG(st.st_mode)) {
        http_sendfile(fd, tx);
        return;
    }

    if (!S_ISDIR(st.st_mode)) {
        http_forbidden(tx);
        if (close(fd)) perror("close");
        return;
    }

    /* display index.html if exists */
    if (check_index(tx, fd)) {
        if (close(fd)) perror("close");
        return;
    }

    http_forbidden(tx);
    if (close(fd)) perror("close");
}
