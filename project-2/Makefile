
CC = gcc
CFLAGS = -g -Wall -std=c89 -ansi -pedantic
LIBS = libs/usr_packet.c libs/usr_packet.h

all:  cclient server

cclient: src/cclient.c src/cclient.h
	$(CC) $(CFLAGS) -o $@ src/cclient.c $(LIBS)

server: src/server.c src/server.h
	$(CC) $(CFLAGS) -o $@ src/server.c $(LIBS)

clean:
	rm -rf cclient server *.dSYM
