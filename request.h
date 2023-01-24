// SPDX-License-Identifier: MIT

#ifndef REQUEST_H
#define REQUEST_H

int init_request(char *arg);

void handle_request(FILE *rx, FILE *tx);

#endif
