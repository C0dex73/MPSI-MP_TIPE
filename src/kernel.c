#include <math.h>
#include "config.h"
#include "kernel.h"

#ifdef TIPE_SHOWKERNEL
int main() {

}
#endif

float kernelF(float radius) {
    return expf(4*(1-1/(4*radius*(1-radius))));
}