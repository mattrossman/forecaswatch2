#pragma once

#include <pebble.h>

#include "stats_metrics.h"

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
    /* Append new fields at the END only. persist_read_data copies
       min(stored, sizeof(Config)) bytes, but an older blob also carries its
       trailing padding, which can overlap new fields. config_read_or_default
       resets fields the stored blob did not fully cover; extend that check
       when appending. */
    bool top_band_stats;
    uint8_t stat_slots[STATS_MAX_SLOTS];
} Config;

extern Config *g_config;

/* Exported so persist_init can seed the same defaults instead of duplicating
   them, which is how the two lists used to drift. */
Config config_defaults(void);

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
