// SPDX-License-Identifier: MIT

#ifndef RBUFFER_H
#define RBUFFER_H

typedef struct rbuffer_s rbuffer_s;

rbuffer_s *rbuffer_create(int size);

void rbuffer_destroy(rbuffer_s *rb);

void rbuffer_put(rbuffer_s *rb, int value);

int rbuffer_get(rbuffer_s *rb);

#endif
