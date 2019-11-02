#pragma once

#include <pebble.h>

void loading_layer_create(Layer* parent_layer, GRect frame);

void loading_layer_set_hidden(bool hidden);

void loading_layer_destroy();