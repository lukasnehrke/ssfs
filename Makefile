CC      = gcc
CFLAGS  = -std=c11 -pedantic -Wall -Werror -Wextra -D_XOPEN_SOURCE=700
RM      = rm -f

server: server.o
