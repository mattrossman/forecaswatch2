#pragma once

#include <pebble.h>

/* The battery gauge, shared by the calendar status row and the stats grid.
   Battery is persistent status rather than a metric, so the grid draws it too
   instead of making the user spend one of their tiles on it. */

#define BATTERY_INDICATOR_W 29
#define BATTERY_INDICATOR_H 10

#ifdef PBL_COLOR
/* Green/yellow/red for a 0-100 remaining level; also used by the stats grid's
   level bars so both read identically. */
GColor battery_indicator_level_color(int level);
#endif

/* Draws the gauge (and the charging bolt when plugged in) to fill `frame`. */
void battery_indicator_draw(GContext *ctx, GRect frame);

/* Width, from the right edge of a `frame_w` wide frame, that the drawing actually
   covers: the full frame while the charging bolt shows, otherwise just the gauge.
   The frame's left part is left blank for the bolt when not charging. */
int battery_indicator_occupied_width(int frame_w);

/* Releases the charging-bolt bitmap. Safe to call repeatedly. */
void battery_indicator_unload(void);
