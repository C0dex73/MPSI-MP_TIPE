#include "utils.h"
#include "random.h"
#include "config.h"
#include "matrix.h"
#include <stdio.h>

struct Cell matrix[MATRIXWIDTH][MATRIXHEIGTH];

void clearMatrix() {
    for(unsigned int i = 0; i < MATRIXWIDTH; ++i) {
        for(unsigned int j = 0; j < MATRIXHEIGTH; ++j) {
            matrix[i][j].oldState = .0f;
            matrix[i][j].state = .0f;
        }
    }
}

void randomizeMatrix() {
    for(unsigned int i = 0; i <= MATRIXWIDTH/(KERNELRAD*RDMDENSITY); ++i) {
        //choose center of patch's coordinates
        unsigned int x = normalizedRandom()*(float)MATRIXWIDTH, y = normalizedRandom()*(float)MATRIXHEIGTH;
        //randomize patch
        for(int i = x-KERNELRAD; i <= x+KERNELRAD; ++i) {
            for (int j = y-KERNELRAD ; j <= y+KERNELRAD ; ++j) {
                matrix[loopback(i, MATRIXWIDTH-1)][loopback(j, MATRIXHEIGTH-1)].state = normalizedRandom();
                matrix[loopback(i, MATRIXWIDTH-1)][loopback(j, MATRIXHEIGTH-1)].oldState = normalizedRandom();
            }
        }
    }
    for(int i = 0; i <= MATRIXWIDTH; ++i) {
        for(int j = 0; i <= MATRIXHEIGTH; ++i) {
            printf("%f ", matrix[i][j].state);
        }
        printf("\n");
    }
}