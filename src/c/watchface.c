#include <pebble.h>
#include "windows/main_window.h"
#include "appendix/define_globals.h"
#include "appendix/globals.h"
#include "layers/forecast_layer.h"
#include "layers/weather_status_layer.h"
#include "appendix/persist.h"
#include "appendix/math.h"

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Message received!");
    Tuple *array_tuple = dict_find(iterator, MESSAGE_KEY_ARRAY);
    Tuple *temp_start_tuple = dict_find(iterator, MESSAGE_KEY_TEMP_START);
    Tuple *city_tuple = dict_find(iterator, MESSAGE_KEY_CITY);

    if(array_tuple && temp_start_tuple && city_tuple) {
        APP_LOG(APP_LOG_LEVEL_INFO, "All tuples received!");
        persist_set_temp_start((int)temp_start_tuple->value->int32);
        int16_t *data = (int16_t*) array_tuple->value->data;
        persist_set_temp_trend(data, 12);
        persist_set_city((char*)city_tuple->value->cstring);
        int lo, hi;
        min_max(data, 12, &lo, &hi);
        persist_set_temp_lo(lo);
        persist_set_temp_hi(hi);
        forecast_layer_refresh();
        weather_status_layer_refresh();
    }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void init() {
    // Register callbacks
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);

    persist_init();

    // Open AppMessage
    const int inbox_size = 128;
    const int outbox_size = 0;
    app_message_open(inbox_size, outbox_size);

    main_window_create();
}

static void deinit() {
    main_window_destroy();
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
