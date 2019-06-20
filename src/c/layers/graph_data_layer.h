#pragma once

#include <pebble.h>

void graph_data_layer_create(Layer* parent_layer, GRect frame);

void graph_data_layer_refresh();

void graph_data_layer_destroy();