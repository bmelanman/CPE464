# Makefile for CPE464 tcp and udp test code
# Created by Hugh Smith
# Updated by Bryce Melander - May 2023

# all target makes UDP test code

CC= gcc
CFLAGS= -g -Wall -D__LIBCPE464_
LIBS = libcpe464.2.21.a -lstdc++ -ldl

OBJS = networks.o gethostbyname.o safeUtil.o udpPDU.o

all: rcopy server cleano

rcopy: rcopy.c $(OBJS)
	$(CC) $(CFLAGS) -o rcopy rcopy.c $(OBJS) $(LIBS)

server: server.c $(OBJS)
	$(CC) $(CFLAGS) -o server server.c  $(OBJS) $(LIBS)

.c.o:
	gcc -c $(CFLAGS) $< -o $@ $(LIBS)

cleandSYM:
	rm -r *.dSYM || TRUE

cleano: cleandSYM
	rm -f *.o

clean: cleano
	rm -f rcopy server




