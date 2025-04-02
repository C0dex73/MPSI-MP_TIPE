#ifndef TIPE_MATRIX
#define TIPE_MATRIX


#include "config.h"
#include "random.h"

//Declare what a cell is
struct Cell {
    float x, y;
    float state, oldState;
};

//Declare a cell array : the matrix
extern struct Cell matrix[MATRIXWIDTH][MATRIXHEIGTH];

//clears the matrix data
void clearMatrix();

//randomize the matrix by patches
void randomizeMatrix();

#endif