
CC= gcc
CFLAGS= -g -Wall
OBJS = obj/networkUtils.o obj/libPoll.o

all: cclient server cleano

cclient: objDir $(OBJS)
	$(CC) $(CFLAGS) -o cclient cclient.c $(OBJS)

server: objDir $(OBJS)
	$(CC) $(CFLAGS) -o server server.c $(OBJS)

objDir:
	mkdir -p ./obj

%.c.o:
	gcc -c $(CFLAGS) $< -o $@

cleano:
	rm -rf src/*.o lib/*.o *.dSYM

clean: cleano
	rm -f server cclient

