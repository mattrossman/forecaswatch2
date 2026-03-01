#pragma once

#include <pebble.h>

bool dither_is_white(int16_t x, int16_t y, uint8_t gray);

void dither_fill_rect_1bit(GContext *ctx, GRect rect, uint8_t gray);
