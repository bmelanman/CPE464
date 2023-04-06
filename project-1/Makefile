# Example makefile for CPE464 Trace program
#
# Just links in pcap library

CC = gcc
LIBS = libs/checksum.c -lpcap
CFLAGS = -g -Wall -pedantic 
#CGLAGS = 

all:  trace

trace: trace.c libs/checksum.c
	$(CC) $(CFLAGS) -o $@ trace.c $(LIBS)

clean:
	rm -rf trace *.dSYM
