// SPDX-License-Identifier: MIT

#ifndef REQUEST_H
#define REQUEST_H

int init_request(void);

void shutdown_request(void);

void handle_request(FILE *rx, FILE *tx);

#endif
