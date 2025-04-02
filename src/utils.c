#include "utils.h"

int loopback(int index, int len) {
    if (index > len) return index - len - 1;
    if (index < 0) return len + index + 1;
    return index;
}