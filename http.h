// SPDX-License-Identifier: MIT

#ifndef HTTP_H
#define HTTP_H

void http_sendfile(int from, FILE *to);

void http_ok(FILE *tx);

void http_bad_request(FILE *tx);

void http_forbidden(FILE *tx);

void http_not_found(FILE *tx);

void http_internal_server_error(FILE *tx);

#endif
