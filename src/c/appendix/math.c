#include "math.h"
#include <pebble.h>
#include "config.h"
#include "persist.h"

void min_max(int16_t *array, int n, int *min, int *max) {
    // It is assumed that the array has at least one element
    *min = array[0];
    *max = array[0];
    for (int i = 1; i < n; ++i) {
        if (array[i] < *min) {
            *min = array[i];
        }
        else if (array[i] > *max) {
            *max = array[i];
        }
    }
}

int roundFloat(float num) 
{ 
    return num < 0 ? num - 0.5 : num + 0.5; 
} 
  

int f_to_c(int temp_f) {
    // Convert a fahrenheit temperature to celcius
    return roundFloat((temp_f - 32) * 5.0 / 9);
}