#pragma once

#include <pebble.h>

typedef struct {
    bool celsius;
} Config;

int config_localize_temp(int temp_f);