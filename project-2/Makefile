
CC = gcc
CFLAGS = -g -Wall -std=c99 -pedantic
LIBS = networkUtils.c serverTable.c libPoll.c

all:  cclient server

cclient: cclient.c
	$(CC) $(CFLAGS) -o $@ cclient.c $(LIBS)

server: server.c
	$(CC) $(CFLAGS) -o $@ server.c $(LIBS)

clean:
	rm -rf cclient server *.dSYM

cleano:
	rm -f *.o
