// SPDX-License-Identifier: MIT

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

int send_file(int from, FILE *to) {
    char buffer[BUFSIZ];

    ssize_t bytes;
    while ((bytes = read(from, buffer, sizeof(buffer))) > 0) {
        if (fwrite(buffer, 1, bytes, to) < (size_t) bytes) {
            int err = errno;
            close(from);
            errno = err;
            return -1;
        }
    }

    if (bytes < 0) {
        int err = errno;
        close(from);
        errno = err;
        return -1;
    }

    if (close(from)) return -1;
    return 0;
}
