
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include "cclient.h"

int main(int argc, char **argv) {

    int cli_sock;

    if (argc != 4) return 1;

    if (strlen(argv[1]) > 100) {
        fprintf(stderr, "Invalid handle, handle longer than 100 characters: %s", argv[1]);
        return 1;
    } else if (!isalpha(argv[1][0])) {
        fprintf(stderr, "Invalid handle, handle starts with a number");
        return 1;
    }

    cli_sock = network_init(argv[1], argv[2], argv[3]);

    if (cli_sock < 0) return 1;

    network_run(cli_sock);

    return 0;
}

int network_init(char *server_name, char *handle, char *server_port) {

    int sock_fd, err;
    struct addrinfo *info, hints = {
            0,
            AF_INET6,
            SOCK_STREAM,
            IPPROTO_IP,
            0,
            "",
            NULL,
            NULL
    };

    printf(
            "\n"
            "Handle: %s\n"
            "Server Name: %s\n"
            "Server Port: %s\n"
            "\n",
            handle, server_name, server_port
    );

    /* Get hostname of server_net_init */
    if ((err = getaddrinfo(server_name, server_port, &hints, &info)) != 0) {
        fprintf(stderr, "getaddrinfo() err #%d:\n", err);
        fprintf(stderr, "%s\n", gai_strerror(err));
        return -1;
    }

    /* Open a new socket */
    sock_fd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if (sock_fd == -1) {
        fprintf(stderr, "Couldn't open socket!\n");
        return -1;
    }

    /* Connect the socket */
    if (connect(sock_fd, info->ai_addr, info->ai_addrlen) != 0) {
        fprintf(stderr, "Could not connect to server_net_init!\n");
        return -1;
    }

    /* Clean up */
    freeaddrinfo(info);

    return sock_fd;

}

void network_run(int client_socket) {

    int exit_flag = 1;
    char buff[BUFF_SIZE] = "";

    while (exit_flag) {

        printf("$: ");

        fgets(buff, BUFF_SIZE, stdin);

        /* Check for % at the beginning of each command */
        if (buff[0] != '%') {
            fprintf(stderr, "Invalid command format\n");
            continue;
        }

        /* Parse given command */
        switch (tolower(buff[1])) {
            case 'm':
                message();
                break;
            case 'b':
                broadcast();
                break;
            case 'c':
                multicast();
                break;
            case 'l':
                list();
                break;
            case 'e':
                exit_flag = 0;
                break;
            default:
                fprintf(stderr, "Invalid command\n");
                continue;
        }

    }

    /* Clean up */
    close(client_socket);
}

void message(void) {
    printf("message\n");
}

void broadcast(void) {
    printf("broadcast\n");
}

void multicast(void) {
    printf("multicast\n");
}

void list(void) {
    printf("list\n");
}

/*
 * cclient: This program connects to the server and then sends and receives message from the server.
 *  - Running cclient program has the following format:
 *      $: cclient {handle} {server-name} {server-port}
 *      + handle:      The clients handle (name), max 100 characters
 *      + server-name: The remote machine running the server
 *      + server-port: The port number of the server_net_init application
 *
 *  - The client takes the servers IP address and port number as run time parameters.
 *  - You will also pass in the client’s handle (name) as a run time parameter.
 *
 *  - The client uses the "$: " as a prompt for user input (so you output this to stdout).
 *
 *  - The cclient must support the following commands:
 *      + %M (send message)
 *      + %C (multicast a message)
 *      + %B (broadcast a message)
 *      + %L (get a listing of all handles from the server)
 *      + %E (exit)
 * */

/*
 * Setup:
 *      - Initial setup at the network level
 *      - Refers to the init communication between the client and server_net_init
 *      - connect() and accept()
 * Use:
 *      - Using the network itself
 *      - send() and recv()
 * Teardown:
 *      - Shutting down communications and cleaning things up
 *      - close()
 * */
