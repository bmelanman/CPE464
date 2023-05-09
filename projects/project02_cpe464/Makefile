
CC= gcc
CFLAGS= -g -Wall

OBJS = libPoll.o networkUtils.o serverTable.o

all: cclient server cleano

cclient: cclient.c $(OBJS)
	$(CC) $(CFLAGS) -o cclient cclient.c $(OBJS) $(LIBS)

server: server.c $(SERV_OBJS)
	$(CC) $(CFLAGS) -o server server.c $(OBJS) $(LIBS)

.c.o:
	gcc -c $(CFLAGS) $< -o $@ $(LIBS)

cleano:
	rm -rf *.o *.dSYM

clean:
	rm -rf server cclient *.o *.dSYM




