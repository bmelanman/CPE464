//
// Created by Bryce Melander on 4/29/23.
//

#include <printf.h>
#include <stdlib.h>

#define ROLLING_AVG_LEN 5
#define RNG_MAX 5

uint16_t movingAvg(uint16_t ptrArrNumbers[], uint16_t *sum, uint8_t *pos, uint8_t len, uint32_t nextNum) {

    /* Source: https://gist.github.com/bmccormack/d12f4bf0c96423d03f82 */

    //Subtract the oldest number from the prev sum, add the new number
    *sum = *sum - ptrArrNumbers[*pos] + nextNum;

    //Assign the nextNum to the position in the array
    ptrArrNumbers[*pos] = nextNum;

    if (++(*pos) >= len) {
        *pos = 0;
    }

    //return the average
    return (uint16_t) (*sum / len);
}

int main(void) {

    // the size of this array represents how many numbers will be used
    // to calculate the average
    uint16_t rollingAvg[ROLLING_AVG_LEN] = {0};
    uint16_t sum = 0, avgFreq;
    uint8_t pos = 0;

    int numLoops = 200;

    printf("\n");

    for (int i = 0; i < numLoops; i++) {
        printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
        avgFreq = movingAvg(rollingAvg, &sum, &pos, ROLLING_AVG_LEN, 200 + (rand() % (RNG_MAX + 1)));
        printf("The new average is %d\n", avgFreq);
        for (int j = 0; j < 100000000; ++j);
    }

    return 0;
}
