CC      = gcc
CFLAGS  = -std=c11 -pedantic -Wall -Werror -Wextra -D_XOPEN_SOURCE=700
RM      = rm -f

.PHONY: all clean

all: server

clean:
	$(RM) server *.o

server: server.o connection.o request.o http.o file.o sem.o rbuffer.o
