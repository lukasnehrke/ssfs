// SPDX-License-Identifier: MIT

#ifndef REQUEST_H
#define REQUEST_H

void init_request(char *path);

void shutdown_request(void);

void handle_request(FILE *rx, FILE *tx);

#endif
