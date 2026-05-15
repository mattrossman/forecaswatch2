#pragma once

#include <pebble.h>
#include <time.h>

time_t watch_services_now(void);
struct tm watch_services_localtime(void);
bool watch_services_clock_is_24h_style(void);
BatteryChargeState watch_services_battery_state(void);
bool watch_services_battery_is_fixture(void);
