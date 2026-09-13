#pragma once

#include <pebble.h>

/* The battery gauge, shared by the calendar status row and the stats grid.
   Battery is persistent status rather than a metric, so the grid draws it too
   instead of making the user spend one of their tiles on it. */

#define BATTERY_INDICATOR_W 29
#define BATTERY_INDICATOR_H 10

/* Draws the gauge (and the charging bolt when plugged in) to fill `frame`. */
void battery_indicator_draw(GContext *ctx, GRect frame);

/* Releases the charging-bolt bitmap. Safe to call repeatedly. */
void battery_indicator_unload(void);
