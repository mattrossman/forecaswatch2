#pragma once

#include <pebble.h>

void graph_layer_create(Layer* parent_layer, GRect frame);

void graph_layer_refresh();

void graph_layer_destroy();