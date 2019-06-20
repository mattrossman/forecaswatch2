#include "math.h"

int max(int *array) {
    // It is assumed that the array has at least one element
    int max = array[0];
    int n = sizeof(array)/sizeof(array[0]);
    for (int i = 1; i < n; ++i) {
        if (array[i] > max) {
            max = array[i];
        }
    }
    return max;
}