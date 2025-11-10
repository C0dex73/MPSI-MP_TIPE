#include "dimensions.h"
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int loopback(int value, int max);
float neighbourSum(Dimension *dim, int x, int y);
float kernelF(float radius);
float growth(Dimension *dim, int x, int y);


//calculate a new index as if the arrays were looping end <=> start
DIMAPI int loopback(int index, int len) {
    if (index > len) return index - len - 1;
    if (index < 0) return len + index + 1;
    return index;
}

//tells wether a cell should be alive or not next gen
DIMAPI float growth(Dimension *dim, int x, int y) {
    float sum = neighbourSum(dim, x, y);

    //GAUSSIAN
    float a = dim->a;
    float b = dim->b;
    float c = dim->c;
    float d = dim->d;
    float res = a * expf(-(sum-b)*(sum-b)/(2*c*c))+d;

    return res*dim->DT;
}

//calculates the neighbour sum ponderated by their kernel value relative the the cell in (x,y)
DIMAPI float neighbourSum(Dimension *dim, int x, int y) {
    float sum = .0f;
    for (int i = x-dim->KERNELRAD ; i <= x+dim->KERNELRAD ; ++i){
        for (int j = y-dim->KERNELRAD ; j <= y + dim->KERNELRAD ; ++j) {
            sum += dim->kernel[i-x+dim->KERNELRAD+(j-y+dim->KERNELRAD)*(2*dim->KERNELRAD+1)] * dim->matrix[loopback(i, dim->MATRIXWIDTH-1)+loopback(j, dim->MATRIXHEIGHT-1)*dim->MATRIXWIDTH].oldState;
        }
    }
    return sum/dim->kSum;
}

//generates the kernel matrix, containing the weight of each cells in the neighbour sum
DIMAPI void genKernel(Dimension *dim) {
    for (int i = -dim->KERNELRAD ; i <= dim->KERNELRAD ; ++i){
        for (int j = -dim->KERNELRAD ; j <= dim->KERNELRAD ; ++j) {
            float r = sqrtf(i*i+j*j)/dim->KERNELRAD;
            if (r > 1 || r == 0) {
                dim->kernel[(i+dim->KERNELRAD)*(2*dim->KERNELRAD+1)+(j+dim->KERNELRAD)] = .0f;
                continue;
            }
            float k = kernelF(r);
            dim->kernel[(i+dim->KERNELRAD)*(2*dim->KERNELRAD+1)+(j+dim->KERNELRAD)] = k;
            dim->kSum += k;
        }
    }
}

//The function to apply to a cell's radius to get its kernel factor
DIMAPI float kernelF(float radius) {
    return expf(4*(1-1/(4*radius*(1-radius))));
}

DIMAPI void randomizeDimensionByKernel(Dimension *dim) {
    srand(time(0));
    for(unsigned int i = 0; i < dim->MATRIXWIDTH; ++i) {
        for(unsigned int j = 0; j < dim->MATRIXHEIGHT; ++j) {
            dim->matrix[i+j*dim->MATRIXWIDTH].oldState = .0f;
            dim->matrix[i+j*dim->MATRIXWIDTH].state = .0f;
            dim->matrixInit[i+j*dim->MATRIXWIDTH].oldState = .0f;
            dim->matrixInit[i+j*dim->MATRIXWIDTH].state = .0f;
        }
    }

    for(unsigned int k = 0; k <= ((float)dim->MATRIXWIDTH*1.f)/((float)dim->KERNELRAD)*dim->RDMDENSITY; ++k) {
        unsigned int x = ((float)rand()/(float)(RAND_MAX))*((float)dim->MATRIXWIDTH), y = ((float)rand()/(float)(RAND_MAX))*((float)dim->MATRIXHEIGHT);
        for(int i = x-dim->KERNELRAD; i <= x+dim->KERNELRAD; ++i) {
            for (int j = y-dim->KERNELRAD ; j <= y+dim->KERNELRAD ; ++j) {
                dim->matrix[loopback(i, dim->MATRIXHEIGHT-1)+loopback(j, dim->MATRIXWIDTH-1)*dim->MATRIXWIDTH].state = ((float)rand()/(float)(RAND_MAX));
                dim->matrix[loopback(i, dim->MATRIXHEIGHT-1)+loopback(j, dim->MATRIXWIDTH-1)*dim->MATRIXWIDTH].oldState = ((float)rand()/(float)(RAND_MAX));
            }
        }
    }
    memcpy(dim->matrixInit, dim->matrix, sizeof(struct Cell)*dim->MATRIXHEIGHT*dim->MATRIXWIDTH);
}

//return the length of the cell array for the specified dimension
DIMAPI unsigned int getMatrixLength(Dimension *dim) {
    return dim->MATRIXWIDTH*dim->MATRIXHEIGHT;
}

DIMAPI Cell *getMatrixInitPointer(Dimension *dim) {
    return dim->matrixInit;
}

DIMAPI Cell *getMatrixPointer(Dimension *dim) {
    return dim->matrix;
}

DIMAPI void noisify(Dimension *dim) {
	srand(0);
	for(unsigned int i = 0; i < dim->MATRIXWIDTH; ++i) {
		for(unsigned int j = 0; j < dim->MATRIXHEIGHT; ++j) {
			dim->matrix[i+j*dim->MATRIXWIDTH].state += (((float)rand())/((float)RAND_MAX)*2.f - 1.f)*dim->noisefactor;
		}
	}
}

//simulation step
DIMAPI void doStep(Dimension *dim) {
    //calculate state from oldState
    for(unsigned int i = 0; i < dim->MATRIXWIDTH; ++i) {
        for(unsigned int j = 0; j < dim->MATRIXHEIGHT; ++j) {
            //DEBUG printf("%f => ", matrix[i][j].state);
            dim->matrix[i+j*dim->MATRIXWIDTH].state += growth(dim, i, j);
            //DEBUG printf("%f => ", matrix[i][j].state);
            if(dim->matrix[i+j*dim->MATRIXWIDTH].state > 1.f) { dim->matrix[i+j*dim->MATRIXWIDTH].state = 1.f; }
            if(dim->matrix[i+j*dim->MATRIXWIDTH].state < 0.f) { dim->matrix[i+j*dim->MATRIXWIDTH].state = 0.f; }
            //DEBUG printf("%f\n", matrix[i][j].state);
        }
    }
    //switch them
    for(unsigned int i = 0; i < dim->MATRIXWIDTH; ++i) {
        for(unsigned int j = 0; j < dim->MATRIXHEIGHT; ++j) {
            dim->matrix[i+j*dim->MATRIXWIDTH].oldState = dim->matrix[i+j*dim->MATRIXWIDTH].state;
        }
    }
}

DIMAPI void printMatrix(Dimension *dim) {
    printf("[");
    for(unsigned int i = 0; i < dim->MATRIXWIDTH-2; ++i) {
        printf("[");
        for(unsigned int j = 0; j < dim->MATRIXHEIGHT-2; ++j) {
            printf("%f,", dim->matrix[i+j*dim->MATRIXWIDTH].state);
        }
        printf("%f],", dim->matrix[i+dim->MATRIXHEIGHT*(dim->MATRIXWIDTH-1)].state);
    }
    printf("[");
        for(unsigned int j = 0; j < dim->MATRIXHEIGHT-2; ++j) {
            printf("%f,", dim->matrix[dim->MATRIXWIDTH+j*dim->MATRIXWIDTH].state);
        }
        printf("%f]]\n", dim->matrix[dim->MATRIXHEIGHT*dim->MATRIXWIDTH-1].state);
}

DIMAPI Dimension *CreateDimension(int w, int h, int cs, int kr, float dt, float rdmd, float a, float b, float c, float d, float nf) {
    static Dimension dim;
    dim.MATRIXWIDTH = w;
    dim.MATRIXHEIGHT = h;
    dim.CELLSIZE = cs;
    dim.KERNELRAD = kr;
    dim.DT = dt;
    dim.RDMDENSITY = rdmd;
    dim.a = a;
    dim.b = b;
    dim.c = c;
    dim.d = d;
    dim.matrix = malloc(w*h*sizeof(struct Cell));
    dim.matrixInit = malloc(w*h*sizeof(struct Cell));
    dim.kernel = malloc((2*kr+1)*(2*kr+1)*sizeof(float));
    dim.noisefactor = nf;

    //matrix and matrixInit initialization
    for(unsigned int i = 0; i < dim.MATRIXWIDTH; ++i) {
        for(unsigned int j = 0; j < dim.MATRIXHEIGHT; ++j) {
            dim.matrix[i+j*dim.MATRIXWIDTH].x = 2.f*(i+.5f)/(dim.MATRIXWIDTH)-1.f;
            dim.matrix[i+j*dim.MATRIXWIDTH].y = 1.f-2.f*(j+.5f)/(dim.MATRIXHEIGHT);
            dim.matrix[i+j*dim.MATRIXWIDTH].state = .0f;
            dim.matrix[i+j*dim.MATRIXWIDTH].oldState = .0f;
        }
    }
    memcpy(dim.matrixInit, dim.matrix, w*h*sizeof(struct Cell));

    //Kernel initialization
    genKernel(&dim);

    return &dim;
}

DIMAPI unsigned int getDimensionCellSize(Dimension *dim) {
    return dim->CELLSIZE;
}

DIMAPI unsigned int getMatrixWidth(Dimension *dim) {
    return dim->MATRIXWIDTH;
}

DIMAPI unsigned int getMatrixHeight(Dimension *dim) {
    return dim->MATRIXHEIGHT;
}
