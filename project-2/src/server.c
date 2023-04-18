
#include "server.h"

/*
 * server: The server acts as a packet forwarder between the cclients.
 *  - The main goal of the server is to forward messages between clients.
 *  - You can view the server as the center of a logical star topology.
 *  - This program is responsible for accepting connections from clients, keeping track of the clients’ handles,
 *    responding to requests for the current list of handles and forwarding messages to the correct client.
 *  - The server program does not terminate (you kill it with a ^c).
 *  - The server program has the following format:
 *      $: server [port-number]
 *      + port-number (optional): If present, it tells the server which port number to use.
 *                                If not present have the OS assign the port number
 *
 *
 * */

int main(void) {
    printf("Hello, World!\n");
    return 0;
}
