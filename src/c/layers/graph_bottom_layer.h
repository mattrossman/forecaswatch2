#pragma once

#include <pebble.h>

void graph_bottom_layer_create(Layer* parent_layer, GRect frame);

void graph_bottom_layer_refresh();

void graph_bottom_layer_destroy();