#include "config.h"
#include "persist.h"
#include "math.h"

int config_localize_temp(int temp_f) {
    // Convert temperatures as desired
    Config *config = (Config*) malloc(sizeof(Config));
    persist_get_config(config);
    int result;
    if (config->celsius)
        result = f_to_c(temp_f);
    else
        result = temp_f;
    free(config);
    return result;
}

int config_format_time(char *s, size_t maxsize, const struct tm * tm_p) {
    Config *config = (Config*) malloc(sizeof(Config));
    persist_get_config(config);
    int res = strftime(s, maxsize, clock_is_24h_style() ? "%H:%M" : "%I:%M", tm_p);
    if (!config->time_lead_zero) {
        // Remove leading zero if configured as such
        if (s[0] == '0') 
            memmove(s, s+1, strlen(s));
    }
    free(config);
    return res;
}

int config_axis_hour(int hour) {
    Config *config = (Config*) malloc(sizeof(Config));
    persist_get_config(config);
    if (config->axis_12h) {
        hour = hour % 12;
        hour = hour == 0 ? 12 : hour;
    }
    else 
        hour = hour % 24;
    free(config);
    return hour;
}