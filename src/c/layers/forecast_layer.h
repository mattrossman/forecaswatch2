#pragma once

#include <pebble.h>

void forecast_layer_create(Layer *parent_layer, GRect frame);

bool forecast_layer_has_valid_data();

void forecast_layer_refresh();

void forecast_layer_destroy();
