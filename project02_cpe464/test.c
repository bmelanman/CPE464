
#include <printf.h>
#include <stdlib.h>
#include <string.h>
#include <arm_neon.h>

#define TEST 16

int main(void) {

    uint32_t var1 = 10101010;

    printf("%f", TEST / (float32_t) var1);

}
