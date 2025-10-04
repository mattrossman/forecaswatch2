#pragma once

#include <pebble.h>

void calendar_status_layer_create(Layer *parent_layer, GRect frame);

void status_icons_refresh();

void bluetooth_icons_refresh(bool connected);

void bluetooth_callback(bool connected);

bool show_qt_icon();

void calendar_status_layer_refresh();

void calendar_status_layer_destroy();