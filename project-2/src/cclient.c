
#include "cclient.h"

int main(int argc, char **argv) {

    uint8_t exit_flag = 1;

    int server_port;
    char handle[100], server_name[20];

    char buff[BUFF_SIZE];

    if (argc != 4) { exit(1); }

    if (strlen(argv[1]) > 100) {
        fprintf(stderr, "Invalid handle, handle longer than 100 characters: %s", argv[1]);
    }

    snprintf(handle, 100, "%s", argv[1]);
    snprintf(server_name, 20, "%s", argv[2]);
    server_port = (int) strtol(argv[3], NULL, 10);

    printf(
            "\n"
            "Handle: %s\n"
            "Server Name: %s\n"
            "Server Port: %d\n"
            "\n",
            handle, server_name, server_port
    );

    while (exit_flag) {

        printf("$: ");

        scanf("%s", buff);

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

    return 0;
}

void parse_input(void) {

}

int network_init(void) {


    return 1;
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
 *      + server-port: The port number of the server application
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
 *      - Refers to the init communication between the client and server
 *      - connect() and accept()
 * Use:
 *      - Using the network itself
 *      - send() and recv()
 * Teardown:
 *      - Shutting down communications and cleaning things up
 *      - close()
 * */
