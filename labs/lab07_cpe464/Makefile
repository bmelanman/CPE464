# Makefile for CPE464 tcp and udp test code
# updated by Hugh Smith - April 2023

# all target makes UDP test code
# tcpAll target makes the TCP test code


CC= gcc
CFLAGS= -g -Wall
LIBS = 

OBJS = networks.o gethostbyname.o pollLib.o safeUtil.o

#uncomment next two lines if your using sendtoErr() library
LIBS += libcpe464.2.21.a -lstdc++ -ldl
CFLAGS += -D__LIBCPE464_


all: udpAll

udpAll: udpClient udpServer cleano
tcpAll: myClient myServer cleano

udpClient: given_files/udpClient.c $(OBJS)
	$(CC) $(CFLAGS) -o udpClient udpClient.c $(OBJS) $(LIBS)

udpServer: given_files/udpServer.c $(OBJS)
	$(CC) $(CFLAGS) -o udpServer udpServer.c  $(OBJS) $(LIBS)

myClient: given_files/myClient.c $(OBJS)
	$(CC) $(CFLAGS) -o myClient myClient.c  $(OBJS) $(LIBS)

myServer: given_files/myServer.c $(OBJS)
	$(CC) $(CFLAGS) -o myServer myServer.c $(OBJS) $(LIBS)

.c.o:
	gcc -c $(CFLAGS) $< -o $@ $(LIBS)

cleandSYM:
	rm -r *.dSYM

cleano: cleandSYM
	rm -f *.o

clean: cleandSYM
	rm -f myServer myClient udpClient udpServer *.o




