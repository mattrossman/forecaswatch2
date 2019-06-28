#include "app_message.h"
#include "persist.h"
#include "math.h"
#include "c/layers/forecast_layer.h"
#include "c/layers/weather_status_layer.h"

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Message received!");
    Tuple *temp_trend_tuple = dict_find(iterator, MESSAGE_KEY_TEMP_TREND_INT16);
    Tuple *precip_trend_tuple = dict_find(iterator, MESSAGE_KEY_PRECIP_TREND_UINT8);
    Tuple *temp_start_tuple = dict_find(iterator, MESSAGE_KEY_TEMP_START);
    Tuple *city_tuple = dict_find(iterator, MESSAGE_KEY_CITY);

    if(temp_trend_tuple && temp_trend_tuple && temp_start_tuple && city_tuple) {
        APP_LOG(APP_LOG_LEVEL_INFO, "All tuples received!");
        persist_set_start_hour((int)temp_start_tuple->value->int32);
        int16_t *temp_data = (int16_t*) temp_trend_tuple->value->data;
        persist_set_temp_trend(temp_data, 12);
        uint8_t *precip_data = (uint8_t*) precip_trend_tuple->value->data;
        persist_set_precip_trend(precip_data, 12);
        APP_LOG(APP_LOG_LEVEL_INFO, "Saving precip data: [%i, %i, %i, ...]", precip_data[0], precip_data[1], precip_data[2]);
        persist_set_city((char*)city_tuple->value->cstring);
        int lo, hi;
        min_max(temp_data, 12, &lo, &hi);
        persist_set_temp_lo(lo);
        persist_set_temp_hi(hi);
        forecast_layer_refresh();
        weather_status_layer_refresh();
    }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

void app_message_init() {
    // Register callbacks
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);

    // Open AppMessage
    const int inbox_size = 128;
    const int outbox_size = 0;
    app_message_open(inbox_size, outbox_size);
}