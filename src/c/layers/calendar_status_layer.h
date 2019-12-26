#pragma once

#include <pebble.h>

void calendar_status_layer_create(Layer* parent_layer, GRect frame);

void status_icons_refresh();

void calendar_status_layer_refresh();

void calendar_status_layer_destroy();