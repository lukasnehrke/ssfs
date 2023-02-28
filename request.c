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
#include "file.h"
#include "http.h"

#define REQ_MAX 8192

static char *base;

static void die(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

static char *parse_request(FILE *rx, FILE *tx) {
    /* read status line */
    char request[REQ_MAX + 1];
    if (fgets(request, sizeof(request), rx) == NULL) {
        if (ferror(rx)) {
            perror("fgets");
            http_internal_server_error(tx);
            return NULL;
        }
        http_bad_request(tx);
        return NULL;
    }

    /* ensure newline is present */
    size_t len = strlen(request);
    if (len == 0 || request[len - 1] != '\n') {
        http_bad_request(tx);
        return NULL;
    }

    /* remove newline */
    if (len > 1 && request[len - 2] == '\r') {
        request[len - 2] = '\0';
    } else {
        request[len - 1] = '\0';
    }

    char *rest;
    char *method   = strtok_r(request, " ", &rest);
    char *path     = strtok_r(NULL, " ", &rest);
    char *protocol = strtok_r(NULL, "", &rest);

    if (method == NULL || path == NULL || protocol == NULL) {
        http_bad_request(tx);
        return NULL;
    }

    /* check for valid method */
    if (strcmp(method, "GET") != 0) {
        http_bad_request(tx);
        return NULL;
    }

    /* check for valid protocol */
    if (strcmp(protocol, "HTTP/1.0") != 0 && strcmp(protocol, "HTTP/1.1") != 0) {
        http_bad_request(tx);
        return NULL;
    }

    /* concat base path and request path */
    size_t base_len = strlen(base);
    char full_path[base_len + strlen(path) + 2];
    if (snprintf(full_path, sizeof(full_path), "%s/%s", base, path) < 0) {
        perror("snprintf");
        http_internal_server_error(tx);
        return NULL;
    }

    /* normalize path */
    char *real_path = realpath(full_path, NULL);
    if (real_path == NULL) {
        if (errno = ENOENT) {
            http_not_found(tx);
            return NULL;
        }
        perror(full_path);
        http_internal_server_error(tx);
        return NULL;
    }

    /* check for path traversal attack */
    if (strncmp(base, real_path, base_len) != 0) {
        http_not_found(tx);
        free(real_path);
        return NULL;
    }

    return real_path;
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

    http_ok(tx);
    if (send_file(fd, tx)) perror("send_file");
    return 1;
}

int init_request(void) {
    const char *path = get_flag("path");
    if (path == NULL) return -1;

    base = realpath(path, NULL);
    if (base == NULL) die(path);

    return 0;
}

void shutdown_request(void) {
    free(base);
}

void handle_request(FILE *rx, FILE *tx) {
    char *path = parse_request(rx, tx);
    if (path == NULL) return;

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        if (errno == ENOENT) {
            http_not_found(tx);
            free(path);
            return;
        }
        if (errno == EACCES) {
            http_forbidden(tx);
            free(path);
            return;
        }
        perror(path);
        http_internal_server_error(tx);
        free(path);
        return;
    }

    free(path);

    struct stat st;
    if (fstat(fd, &st)) {
        perror("fstat");
        http_internal_server_error(tx);
        close(fd);
        return;
    }

    /* send file if regular */
    if (S_ISREG(st.st_mode)) {
        http_ok(tx);
        if (send_file(fd, tx)) perror("send_file");
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
