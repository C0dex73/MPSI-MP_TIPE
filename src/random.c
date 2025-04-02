#include <stdlib.h>
#include <time.h>
#include "random.h"

void initRandom() {
    srand(time(0));
}

float normalizedRandom() {
    return ((float)rand()/(float)(RAND_MAX));
}