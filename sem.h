// SPDX-License-Identifier: MIT

#ifndef SEM_H
#define SEM_H

typedef struct sem_s sem_s;

sem_s *sem_create(int value);

void sem_destroy(sem_s *sem);

void P(sem_s *sem);

void V(sem_s *sem);

#endif
