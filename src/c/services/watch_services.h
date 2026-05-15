#pragma once

#include <pebble.h>
#include <time.h>

time_t watch_services_now(void);
struct tm watch_services_localtime(void);
