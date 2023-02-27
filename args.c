// SPDX-License-Identifier: MIT

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "args.h"

static int num_args;
static char **args;

int parse_args(int argc, char **argv) {
    if (argc < 1 || argv == NULL) {
        errno = EINVAL;
        return -1;
    }
    num_args = argc;
    args = argv;
    return 0;
}

const char *get_flag(const char *flag) {
    if (flag == NULL) return NULL;
    size_t len = strlen(flag);

    for (int i = 1; i < num_args; i++) {
        if (strncmp(args[i], "--", 2) != 0) continue;
        if (strncmp((args[i] + 2), flag, len) != 0) continue;
        if (*(args[i] + len + 2) != '=') continue;
        return args[i] + len + 3;
    }

    return NULL;
}
