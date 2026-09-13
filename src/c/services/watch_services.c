#include "watch_services.h"

#ifdef FCW2_FIXTURE_NOW_YEAR
static bool is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

static int fixture_day_of_year(int year, int month, int day) {
    static const int days_before_month[] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };
    int yday = days_before_month[month - 1] + day - 1;
    if (month > 2 && is_leap_year(year)) {
        yday += 1;
    }

    return yday;
}

static int fixture_weekday(int year, int month, int day) {
    if (month < 3) {
        month += 12;
        year -= 1;
    }

    int k = year % 100;
    int j = year / 100;
    int h = (day + (13 * (month + 1)) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;
    return (h + 6) % 7;
}
#endif

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
        .tm_wday = fixture_weekday(FCW2_FIXTURE_NOW_YEAR, FCW2_FIXTURE_NOW_MONTH, FCW2_FIXTURE_NOW_DAY),
        .tm_yday = fixture_day_of_year(FCW2_FIXTURE_NOW_YEAR, FCW2_FIXTURE_NOW_MONTH, FCW2_FIXTURE_NOW_DAY),
        .tm_isdst = 0
    };
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

BatteryChargeState watch_services_battery_state(void) {
#ifdef FCW2_FIXTURE_BATTERY_PERCENT
    return (BatteryChargeState) {
        .charge_percent = FCW2_FIXTURE_BATTERY_PERCENT,
        .is_charging = FCW2_FIXTURE_BATTERY_CHARGING,
        .is_plugged = FCW2_FIXTURE_BATTERY_CHARGING
    };
#else
    return battery_state_service_peek();
#endif
}

bool watch_services_battery_is_fixture(void) {
#ifdef FCW2_FIXTURE_BATTERY_PERCENT
    return true;
#else
    return false;
#endif
}

#ifdef FCW2_FIXTURE_HEALTH_STEPS
#define FCW2_HEALTH_IS_FIXTURE 1
#endif

#if defined(PBL_HEALTH) && !defined(FCW2_HEALTH_IS_FIXTURE)
/* Accumulating metrics are summed over today; instantaneous ones are peeked.
   Accessibility is checked first so an unsupported sensor reads as unavailable
   rather than a misleading zero. */
static int32_t health_sum_today(HealthMetric metric) {
    const time_t start = time_start_of_today();
    const time_t end = time(NULL);
    if (!(health_service_metric_accessible(metric, start, end) & HealthServiceAccessibilityMaskAvailable)) {
        return WATCH_HEALTH_UNAVAILABLE;
    }

    return (int32_t) health_service_sum_today(metric);
}

/* Average of this metric over the same span on comparable days (weekday vs
   weekend), so the comparison is against the user's own habits. */
static int32_t health_sum_typical(HealthMetric metric) {
    const time_t start = time_start_of_today();
    const time_t end = time(NULL);
    const HealthServiceTimeScope scope = HealthServiceTimeScopeDailyWeekdayOrWeekend;
    if (!(health_service_metric_averaged_accessible(metric, start, end, scope)
            & HealthServiceAccessibilityMaskAvailable)) {
        return WATCH_HEALTH_UNAVAILABLE;
    }

    return (int32_t) health_service_sum_averaged(metric, start, end, scope);
}

static int32_t health_peek(HealthMetric metric) {
    const time_t now = time(NULL);
    if (!(health_service_metric_accessible(metric, now, now) & HealthServiceAccessibilityMaskAvailable)) {
        return WATCH_HEALTH_UNAVAILABLE;
    }

    return (int32_t) health_service_peek_current_value(metric);
}
#endif

int32_t watch_services_health_steps(void) {
#if defined(FCW2_HEALTH_IS_FIXTURE)
    return FCW2_FIXTURE_HEALTH_STEPS;
#elif defined(PBL_HEALTH)
    return health_sum_today(HealthMetricStepCount);
#else
    return WATCH_HEALTH_UNAVAILABLE;
#endif
}

int32_t watch_services_health_distance_meters(void) {
#if defined(FCW2_HEALTH_IS_FIXTURE)
    return FCW2_FIXTURE_HEALTH_DISTANCE_METERS;
#elif defined(PBL_HEALTH)
    return health_sum_today(HealthMetricWalkedDistanceMeters);
#else
    return WATCH_HEALTH_UNAVAILABLE;
#endif
}

MeasurementSystem watch_services_health_distance_units(void) {
#if defined(FCW2_FIXTURE_HEALTH_DISTANCE_UNITS)
    return FCW2_FIXTURE_HEALTH_DISTANCE_UNITS;
#elif defined(FCW2_HEALTH_IS_FIXTURE)
    return MeasurementSystemUnknown;
#elif defined(PBL_HEALTH)
    return health_service_get_measurement_system_for_display(HealthMetricWalkedDistanceMeters);
#else
    return MeasurementSystemUnknown;
#endif
}

int32_t watch_services_health_active_kcalories(void) {
#if defined(FCW2_HEALTH_IS_FIXTURE)
    return FCW2_FIXTURE_HEALTH_ACTIVE_KCALORIES;
#elif defined(PBL_HEALTH)
    return health_sum_today(HealthMetricActiveKCalories);
#else
    return WATCH_HEALTH_UNAVAILABLE;
#endif
}

int32_t watch_services_health_active_minutes(void) {
#if defined(FCW2_HEALTH_IS_FIXTURE)
    return FCW2_FIXTURE_HEALTH_ACTIVE_MINUTES;
#elif defined(PBL_HEALTH)
    {
        const int32_t seconds = health_sum_today(HealthMetricActiveSeconds);
        return seconds == WATCH_HEALTH_UNAVAILABLE ? WATCH_HEALTH_UNAVAILABLE : seconds / 60;
    }
#else
    return WATCH_HEALTH_UNAVAILABLE;
#endif
}

int32_t watch_services_health_active_typical_minutes(void) {
#if defined(FCW2_HEALTH_IS_FIXTURE)
    return FCW2_FIXTURE_HEALTH_ACTIVE_TYPICAL_MINUTES;
#elif defined(PBL_HEALTH)
    {
        const int32_t seconds = health_sum_typical(HealthMetricActiveSeconds);
        return seconds == WATCH_HEALTH_UNAVAILABLE ? WATCH_HEALTH_UNAVAILABLE : seconds / 60;
    }
#else
    return WATCH_HEALTH_UNAVAILABLE;
#endif
}

int32_t watch_services_health_heart_rate(void) {
#if defined(FCW2_HEALTH_IS_FIXTURE)
    return FCW2_FIXTURE_HEALTH_HEART_RATE;
#elif defined(PBL_HEALTH)
    return health_peek(HealthMetricHeartRateBPM);
#else
    return WATCH_HEALTH_UNAVAILABLE;
#endif
}
