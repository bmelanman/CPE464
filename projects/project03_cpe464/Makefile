# Makefile for CPE464 tcp and udp test code
# Created by Hugh Smith - April 2023
# Updated by Bryce Melander - May 2023

# all target makes UDP test code

CC= gcc
CFLAGS= -g -Wall -D__LIBCPE464_ -Wno-attributes
LIBS = libcpe464.2.21.a -lstdc++ -ldl

OBJS = networkUtils.o gethostbyname.o pollLib.o windowLib.o

all: libcpe rcopy server cleano

libcpe:
	make -C ./given_files/ -f ./build464Lib.mk all
	mv ./given_files/*.a ./

rcopy: rcopy.c $(OBJS)
	$(CC) $(CFLAGS) -o rcopy rcopy.c $(OBJS) $(LIBS)

server: server.c $(OBJS)
	$(CC) $(CFLAGS) -o server server.c  $(OBJS) $(LIBS)

.c.o:
	gcc -c $(CFLAGS) $< -o $@

cleandSYM:
	rm -r *.dSYM || true

cleano: cleandSYM
	rm -f *.o

clean: cleano
	rm -f rcopy server *.a




