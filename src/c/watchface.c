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

    if(temp_lo_tuple && temp_hi_tuple) {
        APP_LOG(APP_LOG_LEVEL_INFO, "All tuples received!");
        persist_set_temp_lo((int)temp_lo_tuple->value->int32);
        persist_set_temp_hi((int)temp_hi_tuple->value->int32);
        weather_layer_refresh();
        APP_LOG(APP_LOG_LEVEL_INFO, "New lo: %d, New hi: %d", persist_get_temp_lo(), persist_get_temp_hi());
    }
    if (array_tuple) {
        APP_LOG(APP_LOG_LEVEL_INFO, "Array tuple received!");
        int16_t *data = (int16_t*) array_tuple->value->data;
        APP_LOG(APP_LOG_LEVEL_INFO, "First int: %i", data[0]);
        APP_LOG(APP_LOG_LEVEL_INFO, "Second int: %i", data[1]);
        APP_LOG(APP_LOG_LEVEL_INFO, "Third int: %i", data[2]);
    }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init() {
    // Register callbacks
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);

    persist_init();

    // Open AppMessage
    const int inbox_size = 128;
    const int outbox_size = 128;
    app_message_open(inbox_size, outbox_size);
    APP_LOG(APP_LOG_LEVEL_INFO, "Outbox opened!");

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
