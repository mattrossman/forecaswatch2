#pragma once

#include <pebble.h>

void calendar_layer_create(Layer *parent_layer, GRect frame);

void calendar_layer_refresh();

void calendar_layer_destroy();