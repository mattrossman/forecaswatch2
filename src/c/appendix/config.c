#include "config.h"
#include "persist.h"
#include "globals.h"
#include "math.h"

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

GColor config_today_color() {
    GColor color = g_config->color_today;
    return color;
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

bool config_show_qt() {
    bool show_qt = g_config->show_qt;
    return show_qt;
}

bool config_show_bt() {
    bool show_bt = g_config->show_bt;
    return show_bt;
}

bool config_show_bt_disconnect() {
    bool show_bt_disconnect = g_config->show_bt_disconnect;
    return show_bt_disconnect;
}

bool config_vibe() {
    bool vibe = g_config->vibe;
    return vibe;
}