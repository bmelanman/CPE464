
#include <printf.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MSG 20
#define MAX_PKTS 7

int chopUsrMessage(char usrMessage[], char messages[][MAX_MSG]) {

    uint8_t i, packetLen;
    uint16_t offset = 0;

    for (i = 0; i < MAX_PKTS; i++) {
        packetLen = strnlen(&usrMessage[offset], MAX_MSG - 1);

        if (packetLen < 1) break;

        snprintf(messages[i], MAX_MSG, "%s", &usrMessage[offset]);

        offset += packetLen;
    }

    return i;
}

int main(void) {

    char *input = "aaa bbb ccc ddd eee fff ggg";
    char messages[MAX_PKTS][MAX_MSG];

    int numMessages = chopUsrMessage(input, messages);

    for (int i = 0; i < numMessages; i++) {
        printf("%s\n", messages[i]);
    }

    return 0;

    //hello the
    //re! my na
    //me is bry
    //ce :)

}
