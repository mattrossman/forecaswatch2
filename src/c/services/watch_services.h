#pragma once

#include <pebble.h>
#include <time.h>

time_t watch_services_now(void);
struct tm watch_services_localtime(void);
bool watch_services_clock_is_24h_style(void);
BatteryChargeState watch_services_battery_state(void);
bool watch_services_battery_is_fixture(void);

/* Health reads. Each returns WATCH_HEALTH_UNAVAILABLE when the metric cannot be
   read: no health support (aplite), no permission, or no sensor. Fixture builds
   return their configured values so the emulator, which supplies no health data,
   still renders something deterministic. */
#define WATCH_HEALTH_UNAVAILABLE (-1)

int32_t watch_services_health_steps(void);
int32_t watch_services_health_distance_meters(void);
/* The distance unit the user picked in the Pebble app's Health settings.
   MeasurementSystemUnknown when none was picked or health is unavailable. */
MeasurementSystem watch_services_health_distance_units(void);
int32_t watch_services_health_active_kcalories(void);
int32_t watch_services_health_active_minutes(void);
/* What this user typically has done by this time on a comparable day, which is
   the basis Pebble Health uses for its "% of typical" comparisons. */
int32_t watch_services_health_active_typical_minutes(void);
int32_t watch_services_health_heart_rate(void);

/* The last hour of heart rate, oldest first, folded into `count` equal buckets
   holding the average BPM of that bucket's readings (0 where there were none).
   Reads only what the firmware already logged; it never asks the sensor for
   more samples. Returns how many buckets hold a reading, or
   WATCH_HEALTH_UNAVAILABLE. Costs a firmware call, so callers should cache.
   emery: only built there, as the heart rate graph is its sole caller. */
#ifdef PBL_PLATFORM_EMERY
#define WATCH_HR_HISTORY_MINUTES 60
int watch_services_health_heart_rate_history(uint8_t *points, int count);
#endif
