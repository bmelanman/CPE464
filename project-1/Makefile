
CC = gcc
CFLAGS = -g -Wall -std=c89 -ansi -pedantic
LIBS = libs/checksum.c -lpcap

all:  trace

trace: trace.c libs/checksum.c
	$(CC) $(CFLAGS) -o $@ trace.c $(LIBS)

clean:
	rm -rf trace *.dSYM
