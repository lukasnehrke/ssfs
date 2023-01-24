// SPDX-License-Identifier: MIT

#ifndef FILE_H
#define FILE_H

char *resolve_path(const char *base, const char *path);

FILE *open_file(char *path);

int send_file(FILE *file, FILE *tx);

#endif
