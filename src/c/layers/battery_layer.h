#pragma once

#include <pebble.h>

void battery_layer_create(Layer* parent_layer, GRect frame);

void battery_layer_refresh();

void battery_layer_destroy();