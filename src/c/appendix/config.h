#pragma once

#include <pebble.h>

enum TimeFont {
    TIME_FONT_ROBOTO = 0,
    TIME_FONT_LECO = 1,
    TIME_FONT_BITHAM = 2,
};

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
    GColor color_today;
    GColor color_saturday;
    GColor color_sunday;
    GColor color_us_federal;
    GColor color_time;
    bool day_night_shading;
    bool precip_amount_bars;
    // Config is persisted as a raw blob. The previous layout ended at byte 19 with one byte of tail
    // padding (sizeof == 20), so a bool placed right after it would read that leftover byte on
    // upgrade. Declaring the int16_t first pushes both new fields past the old end (offsets 20, 22),
    // so upgraded installs get the defaults from config_defaults().
    int16_t uv_threshold;
    bool show_uv;
} Config;

extern Config *g_config;

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
