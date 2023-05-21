// Hugh Smith - April 2017 

// Replacement code for gethostbyname 
// Gives either IPv4 address, IPv6 address or IPv4 mapped IPv6 address
// Works well with sockets of type family AF_INET6                        

#ifndef GETHOSTBYNAME_H
#define GETHOSTBYNAME_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

unsigned char *gethostbyname6(const char *hostName, struct sockaddr_in6 *aSockaddr6);

char *ipAddressToString(struct sockaddr_in6 *ipAddressStruct);

#endif