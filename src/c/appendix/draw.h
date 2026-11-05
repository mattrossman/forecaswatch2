#pragma once

#include <pebble.h>

/* Diagonal hatch fill, shared by the forecast night shading and the stats
   indicator bars. Hatch phase is derived from absolute coordinates so adjacent
   rects line up into one continuous pattern. */

int16_t draw_hatch_start_y(int16_t x, int16_t y_start, int16_t spacing);

void draw_hatch_rect(GContext *ctx, GRect rect, int16_t spacing);
