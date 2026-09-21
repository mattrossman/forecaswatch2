#pragma once

#include <pebble.h>

#include "c/appendix/stats_metrics.h"

/* Grid of health/weather stat tiles shown instead of the calendar. Absorbs the
   calendar status row as well, so it owns the whole top band. FCW2_STATS_ENABLED
   comes from stats_metrics.h; the stubs below keep main_window free of #ifdefs. */

// emery: the wider 200px screen fits a third tile column.
#ifdef PBL_PLATFORM_EMERY
#define STATS_COLS 3
#else
#define STATS_COLS 2
#endif
#define STATS_ROWS 2

/* Slots actually visible on this platform; STATS_MAX_SLOTS (stats_metrics.h) is
   the persisted array size and is the same everywhere. */
#define STATS_SLOT_COUNT (STATS_COLS * STATS_ROWS)

bool stats_layer_enabled(void);

void stats_layer_create(Layer *parent_layer, GRect frame);

void stats_layer_set_hidden(bool hidden);

void stats_layer_refresh(void);

void stats_layer_destroy(void);
