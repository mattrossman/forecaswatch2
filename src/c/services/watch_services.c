#include "watch_services.h"
#include <stdlib.h>

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

// emery: only the heart rate graph on the 3-column grid reads the history.
#ifdef PBL_PLATFORM_EMERY

#if defined(FCW2_HEALTH_IS_FIXTURE) || defined(PBL_HEALTH)
typedef struct {
    uint16_t sum[WATCH_HR_HISTORY_MINUTES];
    uint8_t n[WATCH_HR_HISTORY_MINUTES];
} HeartRateBuckets;

/* `minute` counts from the start of the hour-long window; readings outside it
   and minutes without a reading are ignored. */
static void hr_buckets_add(HeartRateBuckets *buckets, int count, int minute, int bpm) {
    if (minute < 0 || minute >= WATCH_HR_HISTORY_MINUTES || bpm <= 0) {
        return;
    }

    const int index = (minute * count) / WATCH_HR_HISTORY_MINUTES;
    buckets->sum[index] += (uint16_t) bpm;
    buckets->n[index] += 1;
}

static int hr_buckets_finish(const HeartRateBuckets *buckets, uint8_t *points, int count) {
    int valid = 0;
    for (int i = 0; i < count; ++i) {
        points[i] = buckets->n[i] ? (uint8_t) (buckets->sum[i] / buckets->n[i]) : 0;
        if (points[i]) {
            ++valid;
        }
    }

    return valid;
}
#endif

int watch_services_health_heart_rate_history(uint8_t *points, int count) {
    if (count <= 0 || count > WATCH_HR_HISTORY_MINUTES) {
        return WATCH_HEALTH_UNAVAILABLE;
    }

#if defined(FCW2_HEALTH_IS_FIXTURE)
#ifdef FCW2_FIXTURE_HEALTH_HR_HISTORY
    /* Fixture minutes end at "now", so a short list fills the newest minutes. */
    static const uint8_t fixture_minutes[] = FCW2_FIXTURE_HEALTH_HR_HISTORY;
    const int len = (int) ARRAY_LENGTH(fixture_minutes);
    HeartRateBuckets buckets = {0};
    for (int i = 0; i < len; ++i) {
        hr_buckets_add(&buckets, count, WATCH_HR_HISTORY_MINUTES - len + i, fixture_minutes[i]);
    }
    return hr_buckets_finish(&buckets, points, count);
#else
    (void) points;
    return WATCH_HEALTH_UNAVAILABLE;
#endif
#elif defined(PBL_HEALTH)
    const time_t window_start = watch_services_now() - WATCH_HR_HISTORY_MINUTES * 60;
    time_t start = window_start;
    time_t end = window_start + WATCH_HR_HISTORY_MINUTES * 60;
    if (!(health_service_metric_accessible(HealthMetricHeartRateBPM, start, end)
          & HealthServiceAccessibilityMaskAvailable)) {
        return WATCH_HEALTH_UNAVAILABLE;
    }

    /* Heap, not stack: 60 records are about a kilobyte and only live for this call. */
    HealthMinuteData *minutes = malloc(sizeof(HealthMinuteData) * WATCH_HR_HISTORY_MINUTES);
    if (!minutes) {
        return WATCH_HEALTH_UNAVAILABLE;
    }

    const uint32_t returned = health_service_get_minute_history(minutes, WATCH_HR_HISTORY_MINUTES,
                                                                &start, &end);
    HeartRateBuckets buckets = {0};
    /* The firmware may hand back a shifted range, so place each record by the
       start time it reports rather than by where it was asked to begin. */
    const int first_minute = (int) (start - window_start) / 60;
    for (uint32_t i = 0; i < returned; ++i) {
        if (!minutes[i].is_invalid) {
            hr_buckets_add(&buckets, count, first_minute + (int) i, minutes[i].heart_rate_bpm);
        }
    }
    free(minutes);
    return hr_buckets_finish(&buckets, points, count);
#else
    (void) points;
    return WATCH_HEALTH_UNAVAILABLE;
#endif
}

#endif /* PBL_PLATFORM_EMERY */
