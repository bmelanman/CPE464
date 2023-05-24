
CC = gcc
CFLAGS = -g -Wall -pedantic -std=c89
LIBS = libs/checksum.c -lpcap

all:  trace

trace: trace.c libs/checksum.c
	$(CC) $(CFLAGS) -o $@ trace.c $(LIBS)

clean:
	rm -rf trace *.dSYM
