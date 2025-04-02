#ifndef TIPE_CONFIG
#define TIPE_CONFIG

// presets : 120x120|5  /   960x505|2
#define MATRIXWIDTH 120
#define MATRIXHEIGTH 120
#define CELLSIZE 5
#define STR_(X) #X
#define STR(X) STR_(X)
#define KERNELRAD 13.0f
#define DT .1f
#define RDMDENSITY 5

//#define SHOWKERNEL


#ifdef SHOWKERNEL
#undef MATRIXWIDTH
#define MATRIXWIDTH (2*(int)KERNELRAD+1)
#undef MATRIXHEIGTH
#define MATRIXHEIGTH (2*(int)KERNELRAD+1)
#undef CELLSIZE
#define CELLSIZE 10
#endif

#endif