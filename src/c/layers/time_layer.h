#pragma once

#include <pebble.h>

void time_layer_create(Layer* parent_layer, GRect frame);

void time_layer_tick();

void time_layer_refresh();

void time_layer_destroy();