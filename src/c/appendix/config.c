#include "config.h"
#include "persist.h"
#include "math.h"

// NOTE: g_config is a global config variable

static Config config_defaults(void) {
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
        .color_saturday = GColorWhite,
        .color_sunday = GColorWhite,
        .color_us_federal = GColorWhite,
        .color_time = GColorWhite,
        .day_night_shading = true
    };
}

static void config_read_or_default(Config *config) {
    *config = config_defaults();
    persist_get_config(config);
}

void config_load() {
    g_config = (Config*) malloc(sizeof(Config));
    config_read_or_default(g_config);
}

void config_refresh() {
    free(g_config);  // Clear out the old config
    g_config = (Config*) malloc(sizeof(Config));
    config_read_or_default(g_config);  // Then reload
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
    int res = strftime(s, maxsize, clock_is_24h_style() ? "%H:%M" : "%I:%M", tm_p);
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

    time_t today = time(NULL);
    struct tm *tm_today = localtime(&today);
    int wday = tm_today->tm_wday;
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
        [TIME_FONT_LECO] = FONT_KEY_LECO_42_NUMBERS,
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
