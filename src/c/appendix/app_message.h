#pragma once

#include <pebble.h>

void app_message_init();

void app_message_send_startup_state(bool has_forecast_data);

/* Re-sends just the capability report. The startup message races PebbleKit JS
   coming up and is often dropped, so this is sent again once the phone has
   proved it is listening by delivering settings. */
void app_message_send_stats_caps(void);
