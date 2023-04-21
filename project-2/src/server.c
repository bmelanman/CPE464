
#include <netdb.h>
#include "server.h"
#include "../libs/dictionary.h"

/*
 * server: The server acts as a packet forwarder between the cclients.
 *  - The main goal of the server is to forward messages between clients.
 *  - You can view the server as the center of a logical star topology.
 *  - This program is responsible for accepting connections from clients, keeping track of the clients’ handles,
 *    responding to requests for the current list of handles and forwarding messages to the correct client.
 *  - The server program does not terminate (you kill it with a ^c).
 *  - The server program has the following format:
 *      $: server [port-number]
 *      + port-number (optional): If present, it tells the server_net_init which port number to use.
 *                                If not present have the OS assign the port number
 *
 *
 * */

int server_net_init(int port);
int check_listen(int serv_sock);

int main(int argc, char **argv) {

    int port = 0, s_sock, exit_flag = 1;
    dict *handle_dict = create_new_dict(12);

    if (argc > 2) {
        fprintf(stderr, "Invalid number og args!");
        return 1;
    }

    if (argc == 2) {
        port = (int) strtol(argv[1], NULL, 10);
    }

    s_sock = server_net_init(port);

//    while (exit_flag) {
//
//
//    }

//    check_listen(s_sock);

    close(s_sock);

    return 0;
}

int server_net_init(int port) {

    int enable = 1, err;
    int server_sock;
    struct sockaddr_in serv_addr;
    socklen_t len;

    /* Create a socket */
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        printf("Could not create socket!\n");
        return -1;
    }

    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        fprintf(stderr, "setsockopt() \"SO_REUSEADDR\" failed");
        return 1;
    }

    /* Apply the port and accept all local interfaces */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (port > 0) {
        serv_addr.sin_port = htons(port);   /* User specified port */
    } else {
        serv_addr.sin_port = 0;             /* Random port */
    }

    /* Bind the socket into to the socket */
    if (bind(server_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
        printf("Could not bind server_net_init!\n");
        perror("bind err");
        return -1;
    }

    /* Get the port if necessary, otherwise just print it */
    if (port > 0) {
        printf("\nServer Port: %d\n\n", port);
    } else {
        len = sizeof(serv_addr);
        if ((err = getsockname(server_sock, (struct sockaddr *) &serv_addr, &len)) == -1) {
            fprintf(stderr, "getsockname() err #%d:\n", err);
            fprintf(stderr, "%s\n", gai_strerror(err));
            return -1;
        }

        printf("\nServer Port: %d\n\n", ntohs(serv_addr.sin_port));
    }

    return server_sock;

}

int check_listen(int serv_sock) {

    int client_sock;
    struct sockaddr_in peer_info;
    socklen_t len;

    /* Listen for connection requests */
    printf("Listening...\n\n");
    if (listen(serv_sock, DEFAULT_BACKLOG) != 0) {
        printf("Cannot listen for sockets!\n");
        return -1;
    }

    /* Accept the connection request */
    len = sizeof(peer_info);
    client_sock = accept(serv_sock, (struct sockaddr *) &peer_info, &len);
    if (client_sock == -1) {
        fprintf(stderr, "Cannot accept socket connection!\n");
        return -1;
    }

    return client_sock;
}
