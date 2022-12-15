// SPDX-License-Identifier: MIT

#ifndef RB_H
#define RB_H

typedef struct rb_s rb_s;

rb_s *rb_create(int size);

void rb_destroy(rb_s *rb);

void rb_put(rb_s *rb, int value);

int rb_get(rb_s *rb);

#endif
