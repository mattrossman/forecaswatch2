#pragma once

#include <pebble.h>

void weather_layer_create(Layer *parent_layer, GRect frame);

void weather_layer_refresh();

void weather_layer_destroy();