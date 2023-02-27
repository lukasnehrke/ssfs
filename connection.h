// SPDX-License-Identifier: MIT

#ifndef CONNECTION_H
#define CONNECTION_H

int init_connection(void);

void shutdown_connection(void);

void handle_client(int client);

#endif
