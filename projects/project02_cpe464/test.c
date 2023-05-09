
#include <printf.h>
#include <sys/file.h>
#include <unistd.h>
#include <sys/socket.h>

int main(void) {

    int fd = open("textDoc.txt", O_WRONLY);
    char buff[100];

    read(fd, buff, 1);

    socket();

    printf("Hello, balls\n"
           "%s",
           buff
    );

}
