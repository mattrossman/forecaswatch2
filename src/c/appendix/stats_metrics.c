#include "stats_metrics.h"
#include "config.h"
#include "persist.h"
#include "c/layers/loading_layer.h"
#include "c/services/watch_services.h"
#include <string.h>

/* Daily goals for the metrics that draw an indicator bar. The SDK exposes no
   goal API, so these are fixed; a settings input would be the honest fix. */
#define STATS_CALORIE_GOAL 500
#define STATS_UV_BAR_MAX 11

#define METERS_PER_MILE 1609
#define UNAVAILABLE WATCH_HEALTH_UNAVAILABLE

/* Titles stay within 6 characters so any metric fits any slot, including the
   corner tiles that share their title row with the status icons. */
static const char *s_labels[STAT_METRIC_COUNT] = {
    [STAT_METRIC_NONE] = "",
    [STAT_METRIC_STEPS] = "STEPS",
    [STAT_METRIC_DISTANCE] = "DIST",
    [STAT_METRIC_CALORIES] = "KCAL",
    [STAT_METRIC_ACTIVE] = "ACTIVE",
    [STAT_METRIC_HEART_RATE] = "HEART",
    [STAT_METRIC_UV_INDEX] = "UV",
    [STAT_METRIC_BATTERY] = "BATT",
    [STAT_METRIC_CURRENT_TEMP] = "TEMP",
    [STAT_METRIC_SUNRISE] = "RISE",
    [STAT_METRIC_SUNSET] = "SET",
    [STAT_METRIC_DATE] = "DATE",
    [STAT_METRIC_WEEKDAY] = "DAY",
};

/* Clamp before formatting: it keeps every value inside STAT_SAMPLE_TEXT_SIZE and
   stops a nonsense reading from overflowing the distance conversion below. */
static int32_t clamp_i32(int32_t value, int32_t lo, int32_t hi) {
    if (value < lo) {
        return lo;
    }

    return value > hi ? hi : value;
}

static StatSample sample_unavailable(void) {
    StatSample sample = { .bar_pct = STAT_BAR_NONE };
    strncpy(sample.text, "--", sizeof(sample.text) - 1);
    return sample;
}

/* Percentage of a goal, clamped to 0-100. Integer multiply-before-divide keeps
   software division helpers out of the binary (see AGENTS.md and #163). */
static int16_t bar_pct_of(int32_t value, int32_t goal) {
    if (goal <= 0 || value <= 0) {
        return 0;
    }

    const int32_t pct = (value * 100) / goal;
    return (int16_t) (pct > 100 ? 100 : pct);
}

static StatSample sample_steps(void) {
    const int32_t steps = watch_services_health_steps();
    if (steps == UNAVAILABLE) {
        return sample_unavailable();
    }

    const int value = (int) clamp_i32(steps, 0, 999999);
    StatSample sample = { .bar_pct = STAT_BAR_NONE };
    if (value < 10000) {
        snprintf(sample.text, sizeof(sample.text), "%d", value);
    }
    else {
        /* 12345 -> "12.3k", so five digits still fit a narrow tile. */
        snprintf(sample.text, sizeof(sample.text), "%d.%dk", value / 1000, (value % 1000) / 100);
    }
    return sample;
}

static StatSample sample_distance(void) {
    const int32_t meters = watch_services_health_distance_meters();
    if (meters == UNAVAILABLE) {
        return sample_unavailable();
    }

    const int value = (int) clamp_i32(meters, 0, 999999);
    StatSample sample = { .bar_pct = STAT_BAR_NONE };
    /* The unit picked in the Pebble app's Health settings wins; users who never
       picked one get the unit matching their temperature setting. */
    const MeasurementSystem units = watch_services_health_distance_units();
    const bool metric = (units == MeasurementSystemUnknown)
        ? g_config->celsius
        : (units == MeasurementSystemMetric);
    if (metric) {
        /* Tenths via integer division. */
        snprintf(sample.text, sizeof(sample.text), "%d.%dKM", value / 1000, (value % 1000) / 100);
    }
    else {
        const int tenths = (value * 10) / METERS_PER_MILE;
        snprintf(sample.text, sizeof(sample.text), "%d.%dMI", tenths / 10, tenths % 10);
    }
    return sample;
}

static StatSample sample_calories(void) {
    const int32_t kcal = watch_services_health_active_kcalories();
    if (kcal == UNAVAILABLE) {
        return sample_unavailable();
    }

    StatSample sample = { .bar_pct = bar_pct_of(kcal, STATS_CALORIE_GOAL) };
    snprintf(sample.text, sizeof(sample.text), "%d", (int) clamp_i32(kcal, 0, 99999));
    return sample;
}

static StatSample sample_active(void) {
    const int32_t minutes = watch_services_health_active_minutes();
    const int32_t typical = watch_services_health_active_typical_minutes();

    /* Shown as a share of what this user typically manages by now, the way
       Pebble Health frames activity. Without history there is nothing to compare
       against, so the tile reports unknown rather than an invented target. */
    if (minutes == UNAVAILABLE || typical == UNAVAILABLE || typical <= 0) {
        return sample_unavailable();
    }

    const int32_t pct = (minutes * 100) / typical;
    const int value = (int) clamp_i32(pct, 0, 999);
    StatSample sample = {
        .bar_pct = (int16_t) clamp_i32(pct, 0, 100),
        .bar_style = STAT_BAR_STYLE_PROGRESS
    };
    snprintf(sample.text, sizeof(sample.text), "%d%%", value);
    return sample;
}

static StatSample sample_heart_rate(void) {
    const int32_t bpm = watch_services_health_heart_rate();
    /* A stale or absent reading shows "--"; a wrong BPM is worse than none. */
    if (bpm == UNAVAILABLE || bpm <= 0) {
        return sample_unavailable();
    }

    StatSample sample = { .bar_pct = STAT_BAR_NONE };
    snprintf(sample.text, sizeof(sample.text), "%d", (int) clamp_i32(bpm, 0, 999));
    return sample;
}

static StatSample sample_battery(void) {
    const BatteryChargeState state = watch_services_battery_state();
    const int percent = (int) clamp_i32(state.charge_percent, 0, 100);
    StatSample sample = { .bar_pct = (int16_t) percent, .bar_style = STAT_BAR_STYLE_LEVEL };
    snprintf(sample.text, sizeof(sample.text), "%d%%", percent);
    return sample;
}

static StatSample sample_uv_index(void) {
    if (!loading_layer_has_valid_data()) {
        return sample_unavailable();
    }

    const int uv = persist_get_uv_index();
    if (uv < 0) {
        return sample_unavailable();
    }

    const int value = (int) clamp_i32(uv, 0, 15);
    StatSample sample = {
        .bar_pct = bar_pct_of(value, STATS_UV_BAR_MAX),
        .bar_style = STAT_BAR_STYLE_PROGRESS
    };
    snprintf(sample.text, sizeof(sample.text), "%d", value);
    return sample;
}

static StatSample sample_current_temp(void) {
    /* Weather-derived metrics go stale with the forecast, so follow the same
       validity check the loading layer uses rather than showing old data. */
    if (!loading_layer_has_valid_data()) {
        return sample_unavailable();
    }

    StatSample sample = { .bar_pct = STAT_BAR_NONE };
    snprintf(sample.text, sizeof(sample.text), "%d°",
             (int) clamp_i32(config_localize_temp(persist_get_current_temp()), -999, 999));
    return sample;
}

static StatSample sample_sun_event(bool want_sunrise) {
    if (!loading_layer_has_valid_data()) {
        return sample_unavailable();
    }

    time_t times[2];
    if (persist_get_sun_event_times(times, 2) != (int) sizeof(times)) {
        return sample_unavailable();
    }

    /* The start type says which event times[0] is; the other one follows it. */
    const bool starts_with_sunrise = (persist_get_sun_event_start_type() == 0);
    const int index = (starts_with_sunrise == want_sunrise) ? 0 : 1;
    struct tm *event_time = localtime(&times[index]);

    StatSample sample = { .bar_pct = STAT_BAR_NONE };
    config_format_time(sample.text, sizeof(sample.text), event_time);
    return sample;
}

static StatSample sample_date(void) {
    struct tm now = watch_services_localtime();
    StatSample sample = { .bar_pct = STAT_BAR_NONE };
    snprintf(sample.text, sizeof(sample.text), "%d", now.tm_mday);
    return sample;
}

static StatSample sample_weekday(void) {
    struct tm now = watch_services_localtime();
    StatSample sample = { .bar_pct = STAT_BAR_NONE };
    strftime(sample.text, sizeof(sample.text), "%a", &now);
    for (char *c = sample.text; *c; ++c) {
        if (*c >= 'a' && *c <= 'z') {
            *c -= ('a' - 'A');
        }
    }
    return sample;
}

uint32_t stats_metrics_caps(void) {
#ifndef FCW2_STATS_ENABLED
    return 0;
#else
    /* Availability, not "has data right now": the health accessors already
       report UNAVAILABLE when a metric is inaccessible, and are fixture-aware. */
    uint32_t caps = 0;

    if (watch_services_health_steps() != UNAVAILABLE) {
        caps |= STATS_CAP_BIT(STAT_METRIC_STEPS);
    }
    if (watch_services_health_distance_meters() != UNAVAILABLE) {
        caps |= STATS_CAP_BIT(STAT_METRIC_DISTANCE);
    }
    if (watch_services_health_active_kcalories() != UNAVAILABLE) {
        caps |= STATS_CAP_BIT(STAT_METRIC_CALORIES);
    }
    if (watch_services_health_active_minutes() != UNAVAILABLE) {
        caps |= STATS_CAP_BIT(STAT_METRIC_ACTIVE);
    }
    if (watch_services_health_heart_rate() != UNAVAILABLE) {
        caps |= STATS_CAP_BIT(STAT_METRIC_HEART_RATE);
    }

    /* Weather- and watch-derived metrics need no sensor, so they are always
       offered; they render "--" until data arrives. */
    caps |= STATS_CAP_BIT(STAT_METRIC_UV_INDEX);
    caps |= STATS_CAP_BIT(STAT_METRIC_BATTERY);
    caps |= STATS_CAP_BIT(STAT_METRIC_CURRENT_TEMP);
    caps |= STATS_CAP_BIT(STAT_METRIC_SUNRISE);
    caps |= STATS_CAP_BIT(STAT_METRIC_SUNSET);
    caps |= STATS_CAP_BIT(STAT_METRIC_DATE);
    caps |= STATS_CAP_BIT(STAT_METRIC_WEEKDAY);

    return caps;
#endif
}

const char *stats_metric_label(StatMetricId id) {
    if (id <= STAT_METRIC_NONE || id >= STAT_METRIC_COUNT) {
        return "";
    }

    return s_labels[id];
}

StatMetricId stats_metric_validate_id(uint8_t id) {
    return (id < STAT_METRIC_COUNT) ? (StatMetricId) id : STAT_METRIC_NONE;
}

StatSample stats_metric_sample(StatMetricId id) {
    switch (id) {
        case STAT_METRIC_STEPS:        return sample_steps();
        case STAT_METRIC_DISTANCE:     return sample_distance();
        case STAT_METRIC_CALORIES:     return sample_calories();
        case STAT_METRIC_ACTIVE:       return sample_active();
        case STAT_METRIC_HEART_RATE:   return sample_heart_rate();
        case STAT_METRIC_BATTERY:      return sample_battery();
        case STAT_METRIC_CURRENT_TEMP: return sample_current_temp();
        case STAT_METRIC_SUNRISE:      return sample_sun_event(true);
        case STAT_METRIC_SUNSET:       return sample_sun_event(false);
        case STAT_METRIC_DATE:         return sample_date();
        case STAT_METRIC_WEEKDAY:      return sample_weekday();
        case STAT_METRIC_UV_INDEX:     return sample_uv_index();
        case STAT_METRIC_NONE:
        default:                       return sample_unavailable();
    }
}
