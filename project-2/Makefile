
CC = gcc
CFLAGS = -g -Wall -pedantic
LIBS = lib/networkUtils.c lib/serverTable.c lib/libPoll.c

all: cclient server

cclient: src/cclient.c
	$(CC) $(CFLAGS) -o $@ src/cclient.c $(LIBS)

server: src/server.c
	$(CC) $(CFLAGS) -o $@ src/server.c $(LIBS)

clean:
	rm -rf cclient server *.dSYM
