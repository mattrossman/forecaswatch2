#pragma once

#include <pebble.h>

/* The stats view needs the health API, so it compiles away entirely on aplite,
   which has neither health nor the RAM to spare. */
#if defined(PBL_HEALTH)
#define FCW2_STATS_ENABLED 1
#endif

/* Metrics selectable for a stats grid slot.
 *
 * These numbers are a wire contract: they are persisted in Config.stat_slots and
 * sent from the phone in CLAY_STATS_SLOTS. Append new metrics at the end and
 * never renumber, exactly as with persist.c's `enum key`. Keep this list in
 * lockstep with src/pkjs/stats-metrics.js.
 */
typedef enum {
    STAT_METRIC_NONE = 0,
    STAT_METRIC_STEPS = 1,
    STAT_METRIC_DISTANCE = 2,
    STAT_METRIC_CALORIES = 3,
    STAT_METRIC_ACTIVE = 4,
    STAT_METRIC_HEART_RATE = 5,
    STAT_METRIC_UV_INDEX = 6,
    STAT_METRIC_BATTERY = 7,
    STAT_METRIC_CURRENT_TEMP = 8,
    STAT_METRIC_SUNRISE = 9,
    STAT_METRIC_SUNSET = 10,
    STAT_METRIC_DATE = 11,
    STAT_METRIC_WEEKDAY = 12,
    STAT_METRIC_COUNT
} StatMetricId;

/* Slots persisted in Config and sent from the phone. Deliberately platform
   independent so Config keeps one layout on every watch. */
#define STATS_MAX_SLOTS 6

#define STAT_SAMPLE_TEXT_SIZE 10
#define STAT_BAR_NONE (-1)

typedef enum {
    /* Filling toward a goal: the bar reads left-to-right as progress. */
    STAT_BAR_STYLE_PROGRESS = 0,
    /* A remaining level, coloured by how much is left, matching the battery
       indicator's existing green/yellow/red thresholds. */
    STAT_BAR_STYLE_LEVEL = 1,
} StatBarStyle;

typedef struct {
    /* Formatted value, or "--" when the metric cannot be read. */
    char text[STAT_SAMPLE_TEXT_SIZE];
    /* 0-100 for metrics with an indicator bar, STAT_BAR_NONE otherwise. */
    int16_t bar_pct;
    StatBarStyle bar_style;
} StatSample;

/* Bit set in the capability mask for a metric this watch can actually provide.
   The mask is 0 on aplite, which has no stats view. */
#define STATS_CAP_BIT(id) (((uint32_t) 1) << (id))

/* Which metrics this watch can supply, sent to the phone so the settings page
   only offers real options. Distinguishes e.g. Pebble 2 HR from Pebble 2 SE,
   which share a platform but not a heart-rate sensor. */
uint32_t stats_metrics_caps(void);

/* Short uppercase tile label. Never NULL. */
const char *stats_metric_label(StatMetricId id);

/* Clamp an untrusted id (from persist or the phone) to a known metric. */
StatMetricId stats_metric_validate_id(uint8_t id);

StatSample stats_metric_sample(StatMetricId id);
