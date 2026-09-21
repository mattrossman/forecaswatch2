#include "config.h"
#include "persist.h"
#include "math.h"
#include "memory_log.h"
#include "c/services/watch_services.h"
#include <stddef.h>
#include <string.h>

Config *g_config;

// Returns defaults as a function (not a static const) because GColor values like
// GColorBlack expand to "compound literals" — C's syntax for inline struct values.
// The C standard doesn't allow these in static variable initializers, so we use a
// function instead. See: https://gcc.gnu.org/onlinedocs/gcc/Compound-Literals.html
Config config_defaults(void) {
    return (Config) {
        .celsius = false,
        .time_lead_zero = false,
        .axis_12h = false,
        .start_mon = false,
        .prev_week = true,
        .show_qt = true,
        .show_bt = true,
        .show_bt_disconnect = true,
        .vibe = false,
        .show_am_pm = false,
        .time_font = TIME_FONT_ROBOTO,
        .color_today = GColorBlack,
        .color_saturday = GColorFolly,
        .color_sunday = GColorFolly,
        .color_us_federal = GColorFolly,
        .color_time = GColorWhite,
        .day_night_shading = true,
        .precip_amount_bars = true,
        .top_band_stats = false,
        /* Slot order matches the settings page: top row then bottom row.
           Heart rate is slot 2, so on watches without the sensor it renders
           "--" until the user picks something else. */
        .stat_slots = {
            STAT_METRIC_DISTANCE,
            STAT_METRIC_HEART_RATE,
            STAT_METRIC_STEPS,
            STAT_METRIC_CALORIES,
            STAT_METRIC_ACTIVE,
            STAT_METRIC_UV_INDEX
        }
    };
}

static void config_read_or_default(Config *config) {
    const Config defaults = config_defaults();
    *config = defaults;
    const int read = persist_get_config(config);

    /* Blobs from before the stats view are 20 bytes: 19 bytes of fields plus a
       padding byte that now holds top_band_stats. That byte was never
       initialised, so a short blob must not decide the new fields. */
    if (read < (int) (offsetof(Config, stat_slots) + sizeof(config->stat_slots))) {
        config->top_band_stats = defaults.top_band_stats;
        memcpy(config->stat_slots, defaults.stat_slots, sizeof(config->stat_slots));
    }
    /* Read the raw byte: testing a bool that holds neither 0 nor 1 is undefined. */
    const uint8_t raw_top_band_stats = ((const uint8_t *) config)[offsetof(Config, top_band_stats)];
    config->top_band_stats = (raw_top_band_stats != 0);
}

void config_load() {
    g_config = (Config*) malloc(sizeof(Config));
    config_read_or_default(g_config);
    MEMORY_LOG_HEAP("after_config_load");
}

void config_refresh() {
    free(g_config);  // Clear out the old config
    g_config = (Config*) malloc(sizeof(Config));
    config_read_or_default(g_config);  // Then reload
    MEMORY_LOG_HEAP("after_config_refresh");
}

void config_unload() {
    free(g_config);
}

int config_localize_temp(int temp_f) {
    // Convert temperatures as desired
    int result;
    if (g_config->celsius)
        result = f_to_c(temp_f);
    else
        result = temp_f;
    return result;
}

int config_format_time(char *s, size_t maxsize, const struct tm * tm_p) {
    int res = strftime(s, maxsize, watch_services_clock_is_24h_style() ? "%H:%M" : "%I:%M", tm_p);
    if (!g_config->time_lead_zero) {
        // Remove leading zero if configured as such
        if (s[0] == '0') 
            memmove(s, s+1, strlen(s));
    }
    return res;
}

int config_axis_hour(int hour) {
    if (g_config->axis_12h) {
        hour = hour % 12;
        hour = hour == 0 ? 12 : hour;
    }
    else 
        hour = hour % 24;
    return hour;
}

int config_n_today() {
    // Returns the index of the calendar box that holds today's date

    struct tm tm_today = watch_services_localtime();
    int wday = tm_today.tm_wday;
    // Offset if user wants to start the week on monday
    wday = g_config->start_mon ? (wday + 6) % 7 : wday;
    // Offset if user wants to show the previous week first
    if (g_config->prev_week)
        wday += 7;
    return wday;
}

GFont config_time_font() {
    const char *font_keys[] = {
        [TIME_FONT_ROBOTO] = FONT_KEY_ROBOTO_BOLD_SUBSET_49,
#ifdef PBL_PLATFORM_EMERY
        // emery: use larger LECO font size
        [TIME_FONT_LECO] = FONT_KEY_LECO_60_NUMBERS_AM_PM,
#else
        [TIME_FONT_LECO] = FONT_KEY_LECO_42_NUMBERS,
#endif
        [TIME_FONT_BITHAM] = FONT_KEY_BITHAM_42_MEDIUM_NUMBERS
    };
    int16_t font_index = g_config->time_font;
    const int16_t font_count = (int16_t)(sizeof(font_keys) / sizeof(font_keys[0]));
    if (font_index < 0 || font_index >= font_count)
        font_index = TIME_FONT_ROBOTO;
    return fonts_get_system_font(font_keys[font_index]);
}

bool config_highlight_holidays() {
    return !gcolor_equal(g_config->color_us_federal, GColorWhite);
}

bool config_highlight_sundays() {
    return !gcolor_equal(g_config->color_sunday, GColorWhite);
}

bool config_highlight_saturdays() {
    return !gcolor_equal(g_config->color_saturday, GColorWhite);
}
