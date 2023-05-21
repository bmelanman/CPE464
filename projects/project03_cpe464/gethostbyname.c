/* Code written by Hugh Smith	April 2017	*/

/* replacement code for gethostbyname - works for IPv4 and IPV6  */
/* Warning - this is NOT thread safe. */

#include "gethostbyname.h"

static unsigned char *getIPAddress46(const char *hostName, struct sockaddr_storage *aSockaddr);

char *ipAddressToString(struct sockaddr_in6 *ipAddressStruct) {
    // puts IP address into a printable format

    static char ipString[INET6_ADDRSTRLEN];

    inet_ntop(AF_INET6, &ipAddressStruct->sin6_addr, ipString, sizeof(ipString));

    return ipString;
}

unsigned char *gethostbyname6(const char *hostName, struct sockaddr_in6 *aSockaddr6) {
    // returns ipv6 address and fills in the aSockaddr6 with address (unless its NULL)
    struct sockaddr_in6 *aSockaddr6Ptr = aSockaddr6;
    struct sockaddr_in6 aSockaddr6Temp;

    // if user does not care about the struct make a temp one
    if (aSockaddr6 == NULL) {
        aSockaddr6Ptr = &aSockaddr6Temp;
    }

    return (getIPAddress46(hostName, (struct sockaddr_storage *) aSockaddr6Ptr));
}

static unsigned char *getIPAddress46(const char *hostName, struct sockaddr_storage *aSockaddr) {
    // Puts host IPv6 (or mapped IPV4) into the aSockaddr6 struct and return pointer to 16 byte address (NULL on error)
    // Only pulls the first IP address from the list of possible addresses

    static unsigned char ipAddress[INET6_ADDRSTRLEN];

    uint8_t *returnValue = NULL;
    int addrError;
    struct addrinfo hints;
    struct addrinfo *hostInfo = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = (AI_V4MAPPED | AI_ALL);
    hints.ai_family = AF_INET6;

    addrError = getaddrinfo(hostName, NULL, &hints, &hostInfo);

    if (addrError != 0) {
        fprintf(stderr, "Error getaddrinfo (host: %s): %s\n", hostName, gai_strerror(addrError));
        returnValue = NULL;
    } else {
        memcpy(
                ((struct sockaddr_in6 *) aSockaddr)->sin6_addr.s6_addr,
                &(((struct sockaddr_in6 *) hostInfo->ai_addr)->sin6_addr.s6_addr),
                16
        );

        memcpy(
                ipAddress,
                &((struct sockaddr_in6 *) aSockaddr)->sin6_addr.s6_addr,
                16
        );

        returnValue = ipAddress;
        freeaddrinfo(hostInfo);
    }

    return returnValue;    // Either Null or IP address
}
