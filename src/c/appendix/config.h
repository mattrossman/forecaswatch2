#pragma once

#include <pebble.h>

typedef struct {
    bool celsius;
    bool time_lead_zero;
    bool axis_12h;
    bool start_mon;
    bool prev_week;
    bool show_qt;
    bool show_bt;
    bool show_bt_disconnect;
    bool vibe;
    bool show_am_pm;
    int16_t time_font;
    int16_t wind_unit; // 0 = mph, 1 = kph
    int16_t wind_max; // 0 = auto, otherwise fixed max value (same unit as wind_unit)
    bool show_wind_graph;
    GColor color_today;
    GColor color_saturday;
    GColor color_sunday;
    GColor color_us_federal;
    GColor color_time;
} Config;

Config *g_config;

void config_load();

void config_refresh();

void config_unload();

int config_localize_temp(int temp_f);

int config_format_time(char *s, size_t maxsize, const struct tm * tm_p);

int config_axis_hour(int hour);

int config_n_today();

GFont config_time_font();

bool config_highlight_holidays();

bool config_highlight_sundays();

bool config_highlight_saturdays();