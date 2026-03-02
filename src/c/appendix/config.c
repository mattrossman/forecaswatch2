#include "config.h"
#include "persist.h"
#include "math.h"

// NOTE: g_config is a global config variable

// Config layout as it existed in master (before wind fields were added).
// Used for migration when upgrading from master -> this branch.
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
} ConfigV1;

static void config_migrate(int bytes_read) {
    if (bytes_read == (int)sizeof(ConfigV1)) {
        // Upgrading from master: persist_read_data loaded 17 bytes at the start
        // of the new 22-byte struct. The 5 GColor fields were at raw offsets
        // 12-16 in ConfigV1, but in Config they are at 16-20 (shifted by the
        // two new int16_t wind fields). Extract the colors from their raw byte
        // positions before we overwrite anything.
        uint8_t *raw = (uint8_t*) g_config;
        GColor old_color_today      = (GColor){ .argb = raw[12] };
        GColor old_color_saturday   = (GColor){ .argb = raw[13] };
        GColor old_color_sunday     = (GColor){ .argb = raw[14] };
        GColor old_color_us_federal = (GColor){ .argb = raw[15] };
        GColor old_color_time       = (GColor){ .argb = raw[16] };
        // Wind fields: safe defaults
        g_config->wind_unit         = 0;    // mph
        g_config->wind_max          = 0;    // auto
        // Restore colors at their correct new offsets
        g_config->color_today       = old_color_today;
        g_config->color_saturday    = old_color_saturday;
        g_config->color_sunday      = old_color_sunday;
        g_config->color_us_federal  = old_color_us_federal;
        g_config->color_time        = old_color_time;
        g_config->show_wind_graph   = true;
    } else if (bytes_read < (int)sizeof(Config)) {
        // Unknown/future layout smaller than current — ensure new fields are sane
        g_config->wind_unit       = 0;
        g_config->wind_max        = 0;
        g_config->show_wind_graph = true;
    }
    // bytes_read == sizeof(Config): current format, nothing to do
}

void config_load() {
    g_config = (Config*) malloc(sizeof(Config));
    int bytes_read = persist_get_config(g_config);
    config_migrate(bytes_read);
}

void config_refresh() {
    free(g_config);  // Clear out the old config
    g_config = (Config*) malloc(sizeof(Config));
    int bytes_read = persist_get_config(g_config);  // Then reload
    config_migrate(bytes_read);
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
        FONT_KEY_ROBOTO_BOLD_SUBSET_49,
        FONT_KEY_LECO_42_NUMBERS,
        FONT_KEY_BITHAM_42_MEDIUM_NUMBERS
    };
    int16_t font_index = g_config->time_font;
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
