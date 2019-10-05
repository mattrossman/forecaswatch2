#pragma once

#include <pebble.h>

typedef struct {
    bool celsius;
    bool time_lead_zero;
} Config;

int config_localize_temp(int temp_f);

int config_format_time(char *s, size_t maxsize, const struct tm * tm_p);