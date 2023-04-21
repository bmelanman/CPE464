# Makefile for CPE464 tcp test code
# written by Hugh Smith - April 2019

CC= gcc
CFLAGS= -g -Wall

OBJS = networks.o gethostbyname.o pollLib.o safeUtil.o libPDU.o

all: cclient server cleano

run_server: server
	./server 54321

run_cclient: cclient
	./cclient localhost 54321

cclient: cclient.c $(OBJS)
	$(CC) $(CFLAGS) -o cclient cclient.c  $(OBJS) $(LIBS)

server: server.c $(OBJS)
	$(CC) $(CFLAGS) -o server server.c $(OBJS) $(LIBS)

%.c.o:
	gcc -c $(CFLAGS) $< -o $@ $(LIBS)

cleano:
	rm -rf *.o *.dSYM

clean:
	rm -rf server cclient *.o




