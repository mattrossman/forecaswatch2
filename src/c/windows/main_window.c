#include "main_window.h"
#include "c/layers/time_layer.h"
#include "c/layers/forecast_layer.h"
#include "c/layers/weather_status_layer.h"
#include "c/layers/calendar_layer.h"
#include "c/layers/calendar_status_layer.h"
#include "c/layers/loading_layer.h"
#include "c/appendix/app_message.h"
#include "c/appendix/persist.h"
#include "c/appendix/memory_log.h"

#define FORECAST_HEIGHT 51
#define WEATHER_STATUS_HEIGHT 14
#define TIME_HEIGHT 45
#define CALENDAR_HEIGHT 45
#define CALENDAR_STATUS_HEIGHT 13

static Window *s_main_window;

static void main_window_load(Window *window) {
    // Get information about the Window
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    int w = bounds.size.w;
    int h = bounds.size.h;
    window_set_background_color(window, GColorBlack);

    forecast_layer_create(window_layer,
            GRect(0, h - FORECAST_HEIGHT, w, FORECAST_HEIGHT));
    weather_status_layer_create(window_layer,
            GRect(0, h - FORECAST_HEIGHT - WEATHER_STATUS_HEIGHT, w, WEATHER_STATUS_HEIGHT));
    time_layer_create(window_layer,
            GRect(0, h - FORECAST_HEIGHT - WEATHER_STATUS_HEIGHT - TIME_HEIGHT,
            bounds.size.w, TIME_HEIGHT));
    calendar_layer_create(window_layer,
            GRect(0, CALENDAR_STATUS_HEIGHT, bounds.size.w, CALENDAR_HEIGHT));
    calendar_status_layer_create(window_layer,
            GRect(0, 0, bounds.size.w, CALENDAR_STATUS_HEIGHT + 1));  // +1 to stop text clipping
    loading_layer_create(window_layer,
            GRect(0, h - FORECAST_HEIGHT - WEATHER_STATUS_HEIGHT, w, FORECAST_HEIGHT + WEATHER_STATUS_HEIGHT));
    loading_layer_refresh();
    app_message_send_startup_state(loading_layer_has_valid_data());
    MEMORY_LOG_HEAP("after_window_load");
}

static void main_window_appear(Window *window) {
    main_window_refresh();
    loading_layer_refresh();
}

static void main_window_unload(Window *window) {
    MEMORY_LOG_HEAP("before_window_unload");
    time_layer_destroy();
    weather_status_layer_destroy();
    forecast_layer_destroy();
    calendar_layer_destroy();
    calendar_status_layer_destroy();
    loading_layer_destroy();
    MEMORY_LOG_HEAP("after_window_unload");
}

static void minute_handler(struct tm *tick_time, TimeUnits units_changed) {
    time_layer_tick();
    /* tm_hour==0 missed day changes from emulator time jumps (same clock, new date). */
    if (units_changed & DAY_UNIT) {
        calendar_layer_refresh();
        calendar_status_layer_refresh();
    }
    status_icons_refresh();
    loading_layer_refresh();
}

/*----------------------------
-------- EXTERNAL ------------
----------------------------*/

void main_window_create() {
    // Create main Window element and assign to pointer
    s_main_window = window_create();

    // Set handlers to manage the elements inside the Window
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = main_window_load,
        .appear = main_window_appear,
        .unload = main_window_unload
    });

    // Register with TickTimerService
    tick_timer_service_subscribe(MINUTE_UNIT | DAY_UNIT, minute_handler);

    // Show the window on the watch with animated=true
    window_stack_push(s_main_window, true);
    time_layer_refresh();
}

void main_window_refresh() {
    time_layer_refresh();
    weather_status_layer_refresh();
    forecast_layer_refresh();
    calendar_layer_refresh();
    calendar_status_layer_refresh();
    loading_layer_refresh();
}

void main_window_destroy() {
    // Interface for destroying the main window (implicitly unloads contents)
    window_destroy(s_main_window);
}
