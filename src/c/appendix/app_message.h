#pragma once

#include <pebble.h>

void app_message_init();

void app_message_send_startup_state(bool has_forecast_data);
