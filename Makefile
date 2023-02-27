CC      = gcc
CFLAGS  = -g -std=c11 -pedantic -Wall -Werror -Wextra -D_XOPEN_SOURCE=700
LDFLAGS = -g
RM      = rm -f

.PHONY: all clean

all: server

clean:
	$(RM) server *.o

server: server.o args.o connection.o request.o http.o file.o sem.o rbuffer.o
