// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "file.h"

char *resolve_path(const char *base, const char *path) {
    char full_path[strlen(base) + strlen(path) + 1];
    strcpy(full_path, base);
    strcat(full_path, path);

    char *real_path = realpath(full_path, NULL);
    if (real_path == NULL) return NULL;

    if (strncmp(base, real_path, strlen(base)) != 0) {
        errno = EACCES;
        return NULL;
    }

    return real_path;
}

FILE *open_file(char *path) {
    /* follows symlinks */
    FILE *file = fopen(path, "r");
    if (file == NULL) return NULL;

    int fd = fileno(file);
    if (fd == -1) {
        int err = errno;
        fclose(file);
        errno = err;
        return NULL;
    }

    struct stat buf;
    if (fstat(fd, &buf)) {
        int err = errno;
        fclose(file);
        errno = err;
        return NULL;
    }

    /* file must be regular */
    if (!S_ISREG(buf.st_mode)) {
        fclose(file);
        errno = EACCES;
        return NULL;
    }

    return file;
}

int send_file(FILE *rx, FILE *tx) {
    char buffer[BUFSIZ];
    while (fgets(buffer, sizeof(buffer), rx) != NULL) {
        if (fputs(buffer, tx) == EOF) {
            return -1;
        }
    }
    if (ferror(rx)) return -1;
    return 0;
}
