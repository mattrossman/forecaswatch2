#include <pebble.h>
#include "windows/main_window.h"
#include "appendix/define_globals.h"
#include "appendix/globals.h"
#include "layers/weather_layer.h"
#include "appendix/persist.h"

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Message received!");
    Tuple *temp_lo_tuple = dict_find(iterator, MESSAGE_KEY_TEMP_LO);
    Tuple *temp_hi_tuple = dict_find(iterator, MESSAGE_KEY_TEMP_HI);
    Tuple *array_tuple = dict_find(iterator, MESSAGE_KEY_ARRAY);
    Tuple *temp_start_tuple = dict_find(iterator, MESSAGE_KEY_TEMP_START);

    if(temp_lo_tuple && temp_hi_tuple && array_tuple && temp_start_tuple) {
        APP_LOG(APP_LOG_LEVEL_INFO, "All tuples received!");
        persist_set_temp_lo((int)temp_lo_tuple->value->int32);
        persist_set_temp_hi((int)temp_hi_tuple->value->int32);
        persist_set_temp_start((int)temp_start_tuple->value->int32);
        int16_t *data = (int16_t*) array_tuple->value->data;
        persist_set_temp_trend(data, 12);
        weather_layer_refresh();
        APP_LOG(APP_LOG_LEVEL_INFO, "New lo: %d, New hi: %d", persist_get_temp_lo(), persist_get_temp_hi());
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
