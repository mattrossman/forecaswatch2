#include "watch_services.h"

time_t watch_services_now(void) {
#ifdef FCW2_FIXTURE_NOW_YEAR
    struct tm fixture_time = watch_services_localtime();
    return mktime(&fixture_time);
#else
    return time(NULL);
#endif
}

struct tm watch_services_localtime(void) {
#ifdef FCW2_FIXTURE_NOW_YEAR
    struct tm fixture_time = (struct tm) {
        .tm_year = FCW2_FIXTURE_NOW_YEAR - 1900,
        .tm_mon = FCW2_FIXTURE_NOW_MONTH - 1,
        .tm_mday = FCW2_FIXTURE_NOW_DAY,
        .tm_hour = FCW2_FIXTURE_NOW_HOUR,
        .tm_min = FCW2_FIXTURE_NOW_MINUTE,
        .tm_sec = FCW2_FIXTURE_NOW_SECOND,
        .tm_isdst = -1
    };
    mktime(&fixture_time);
    return fixture_time;
#else
    time_t now = watch_services_now();
    struct tm *local_time = localtime(&now);
    return *local_time;
#endif
}

bool watch_services_clock_is_24h_style(void) {
#ifdef FCW2_FIXTURE_CLOCK_24H
    return FCW2_FIXTURE_CLOCK_24H;
#else
    return clock_is_24h_style();
#endif
}
