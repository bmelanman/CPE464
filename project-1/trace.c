#include <stdio.h>
#include "libs/checksum.h"

int main() {

    unsigned short *addr = NULL;
    int len = 0;

    printf("Hello, World!\n");
    printf("%d", in_cksum(addr, len));

    return 0;
}
