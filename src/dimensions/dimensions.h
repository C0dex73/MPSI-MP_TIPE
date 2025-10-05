#ifndef __dim_h_
#define __dim_h_

// DLL shenanigans
#ifndef DIMAPI
#  if defined(_WIN32) || defined(__CYGWIN__)
#   if defined(DIM_EXPORT_BUILD)
#    if defined(__GNUC__)
#     define DIMAPI __attribute__ ((dllexport)) extern
#    else
#     define DIMAPI __declspec(dllexport) extern
#    endif
#   else
#    if defined(__GNUC__)
#     define DIMAPI __attribute__ ((dllimport)) extern
#    else
#     define DIMAPI __declspec(dllimport) extern
#    endif
#   endif
#  elif defined(__GNUC__) && defined(DIM_EXPORT_BUILD)
#   define DIMAPI __attribute__ ((visibility ("default"))) extern
#  else
#   define DIMAPI extern
#  endif
#endif

typedef struct Cell {
    float x, y, state, oldState;
} Cell;

typedef struct Dimension {
    int MATRIXWIDTH;
    int MATRIXHEIGHT;
    int CELLSIZE;
    int KERNELRAD;
    float DT;
    float RDMDENSITY;
    Cell* matrix;
    Cell* matrixInit;
    float *kernel;
    float kSum;
    float a;
    float b;
    float c;
    float d;
} Dimension;

DIMAPI Dimension *CreateDimension(int w, int h, int cs, int kr, float dt, float rdmd, float a, float b, float c, float d);
DIMAPI void printMatrix(Dimension *dim);
DIMAPI void doStep(Dimension *dim);
DIMAPI void genKernel(Dimension *dim);
DIMAPI unsigned int getMatrixLength(Dimension *dim);
DIMAPI void randomizeDimensionByKernel(Dimension *dim);
DIMAPI Cell *getMatrixPointer(Dimension *dim);
DIMAPI Cell *getMatrixInitPointer(Dimension *dim);
DIMAPI unsigned int getDimensionCellSize(Dimension *dim);
DIMAPI unsigned int getMatrixWidth(Dimension *dim);
DIMAPI unsigned int getMatrixHeight(Dimension *dim);

#endif // __dim_h_