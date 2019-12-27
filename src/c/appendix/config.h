#pragma once

#include <pebble.h>

typedef struct {
    bool celsius;
    bool time_lead_zero;
    bool axis_12h;
    bool start_mon;
    bool prev_week;
    bool show_qt;
    int16_t time_font;
    GColor color_today;
} Config;

int config_localize_temp(int temp_f);

int config_format_time(char *s, size_t maxsize, const struct tm * tm_p);

int config_axis_hour(int hour);

int config_n_today();

GColor config_today_color();

GFont config_time_font();

bool config_show_qt();