
CC = gcc
CFLAGS = -g -Wall -std=c99 -ansi -pedantic
LIBS = libs/usr_packet.c libs/usr_packet.h

all:  cclient server

cclient: src/cclient.c
	$(CC) $(CFLAGS) -o $@ src/cclient.c $(LIBS)

server: src/server.c
	$(CC) $(CFLAGS) -o $@ src/server.c $(LIBS)

clean:
	rm -rf cclient server *.dSYM
