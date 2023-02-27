CC      = gcc
CFLAGS  = -g -std=c11 -pedantic -Wall -Werror -Wextra -D_XOPEN_SOURCE=700
LDFLAGS = -g
RM      = rm -f

.PHONY: all clean

all: server

clean:
	$(RM) server *.o

server: server.o args.o connection.o request.o http.o sem.o rbuffer.o
args.o: args.c args.h
sem.o: sem.c sem.h
rbuffer.o: rbuffer.c rbuffer.h sem.h
connection.o: connection.c connection.h args.h rbuffer.h request.h
request.o: request.c request.h args.h http.h
http.o: http.c http.h
