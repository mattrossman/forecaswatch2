#include "app_message.h"
#include "persist.h"
#include "math.h"
#include "c/layers/forecast_layer.h"
#include "c/layers/weather_status_layer.h"
#include "c/layers/loading_layer.h"
#include "c/layers/calendar_layer.h"
#include "c/layers/calendar_status_layer.h"
#include "c/windows/main_window.h"
#include "memory_log.h"

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Message received!");
    // Weather data
    Tuple *trends_packed_tuple = dict_find(iterator, MESSAGE_KEY_TRENDS_PACKED_UINT8);
    Tuple *forecast_start_tuple = dict_find(iterator, MESSAGE_KEY_FORECAST_START);
    Tuple *temp_lo_tuple = dict_find(iterator, MESSAGE_KEY_TEMP_LO);
    Tuple *temp_hi_tuple = dict_find(iterator, MESSAGE_KEY_TEMP_HI);
    Tuple *current_temp_tuple = dict_find(iterator, MESSAGE_KEY_CURRENT_TEMP);
    Tuple *city_tuple = dict_find(iterator, MESSAGE_KEY_CITY);
    Tuple *sun_events_tuple = dict_find(iterator, MESSAGE_KEY_SUN_EVENTS);
    Tuple *precip_total_tuple = dict_find(iterator, MESSAGE_KEY_PRECIP_TOTAL_UINT16);
    Tuple *precip_type_tuple = dict_find(iterator, MESSAGE_KEY_PRECIP_TYPE_UINT8);

    // Clay config options
    Tuple *clay_celsius_tuple = dict_find(iterator, MESSAGE_KEY_CLAY_CELSIUS);
    Tuple *clay_time_lead_zero_tuple = dict_find(iterator, MESSAGE_KEY_CLAY_TIME_LEAD_ZERO);
    Tuple *clay_axis_12h_tuple = dict_find(iterator, MESSAGE_KEY_CLAY_AXIS_12H);
    Tuple *clay_start_mon_tuple = dict_find(iterator, MESSAGE_KEY_CLAY_START_MON);
    Tuple *clay_prev_week_tuple = dict_find(iterator, MESSAGE_KEY_CLAY_PREV_WEEK);
    Tuple *clay_color_today_tuple = dict_find(iterator, MESSAGE_KEY_CLAY_COLOR_TODAY);
    Tuple *clay_time_font_tuple = dict_find(iterator, MESSAGE_KEY_CLAY_TIME_FONT);
    Tuple *clay_vibe_tuple = dict_find(iterator, MESSAGE_KEY_CLAY_VIBE);
    Tuple *clay_show_qt_tuple = dict_find(iterator, MESSAGE_KEY_CLAY_SHOW_QT);
    Tuple *clay_show_bt_tuple = dict_find(iterator, MESSAGE_KEY_CLAY_SHOW_BT);
    Tuple *clay_show_bt_disconnect_tuple = dict_find(iterator, MESSAGE_KEY_CLAY_SHOW_BT_DISCONNECT);
    Tuple *clay_show_am_pm_tuple = dict_find(iterator, MESSAGE_KEY_CLAY_SHOW_AM_PM);
    Tuple *clay_color_saturday_tuple = dict_find(iterator, MESSAGE_KEY_CLAY_COLOR_SATURDAY);
    Tuple *clay_color_sunday_tuple = dict_find(iterator, MESSAGE_KEY_CLAY_COLOR_SUNDAY);
    Tuple *clay_color_us_federal_tuple = dict_find(iterator, MESSAGE_KEY_CLAY_COLOR_US_FEDERAL);
    Tuple *clay_color_time_tuple = dict_find(iterator, MESSAGE_KEY_CLAY_COLOR_TIME);
    Tuple *clay_day_night_shading_tuple = dict_find(iterator, MESSAGE_KEY_CLAY_DAY_NIGHT_SHADING);
    Tuple *clay_weather_status_right_mode_tuple = dict_find(iterator, MESSAGE_KEY_CLAY_WEATHER_STATUS_RIGHT_MODE);

    bool has_clay_config = clay_celsius_tuple || clay_time_lead_zero_tuple || clay_axis_12h_tuple || clay_start_mon_tuple
        || clay_prev_week_tuple || clay_color_today_tuple || clay_time_font_tuple || clay_vibe_tuple || clay_show_qt_tuple
        || clay_show_bt_tuple || clay_show_bt_disconnect_tuple || clay_show_am_pm_tuple || clay_color_saturday_tuple
        || clay_color_sunday_tuple || clay_color_us_federal_tuple || clay_color_time_tuple || clay_day_night_shading_tuple
        || clay_weather_status_right_mode_tuple;

    if(trends_packed_tuple && forecast_start_tuple && temp_lo_tuple && temp_hi_tuple && current_temp_tuple && city_tuple && sun_events_tuple && precip_total_tuple && precip_type_tuple) {
        // Weather data received
        APP_LOG(APP_LOG_LEVEL_INFO, "All tuples received!");
        persist_set_forecast_start((time_t)forecast_start_tuple->value->int32);
        const int packed_trend_bytes = (int) trends_packed_tuple->length;
        const int packed_trend_half_bytes = packed_trend_bytes / 2;
        const int num_entries = packed_trend_half_bytes * 2;
        persist_set_num_entries(num_entries);
#ifdef FCW2_ENABLE_MEMORY_LOGGING
        APP_LOG(APP_LOG_LEVEL_DEBUG, "MEM|forecast_payload|entries=%d|free=%lu|used=%lu",
                num_entries,
                (unsigned long)heap_bytes_free(),
                (unsigned long)heap_bytes_used());
#endif
        const int temp_lo = (int) temp_lo_tuple->value->int32;
        const int temp_hi = (int) temp_hi_tuple->value->int32;
        const int temp_range = temp_hi - temp_lo;
        const uint8_t *packed_trend_data = (const uint8_t*) trends_packed_tuple->value->data;
        const uint8_t *packed_temp_data = packed_trend_data;
        const uint8_t *packed_precip_data = packed_trend_data + packed_trend_half_bytes;
        int16_t temp_data[num_entries];
        uint8_t precip_data[num_entries];
        for (int i = 0; i < num_entries; ++i) {
            const uint8_t temp_packed = packed_temp_data[i / 2];
            const uint8_t temp_bucket = (i % 2 == 0) ? (temp_packed & 0x0f) : (temp_packed >> 4);
            const uint8_t precip_packed = packed_precip_data[i / 2];
            const uint8_t precip_bucket = (i % 2 == 0) ? (precip_packed & 0x0f) : (precip_packed >> 4);

            if (temp_range > 0) {
                temp_data[i] = (int16_t)(temp_lo + ((temp_range * temp_bucket + 7) / 15));
            } else {
                temp_data[i] = (int16_t) temp_lo;
            }
            precip_data[i] = (uint8_t)((precip_bucket * 100 + 7) / 15);
        }
        persist_set_temp_trend(temp_data, num_entries);
        persist_set_precip_trend(precip_data, num_entries);
        persist_set_city((char*)city_tuple->value->cstring);
        persist_set_temp_lo(temp_lo);
        persist_set_temp_hi(temp_hi);
        const int current_temp = current_temp_tuple->length == 1
            ? (int) ((int8_t) current_temp_tuple->value->data[0])
            : (int) current_temp_tuple->value->int32;
        persist_set_current_temp(current_temp);
        const uint16_t *sun_event_words = (const uint16_t*) sun_events_tuple->value->data;
        const uint8_t sun_event_start_type = (uint8_t) (sun_event_words[0] >> 15);
        time_t sun_event_times[2];
        sun_event_times[0] = (time_t) forecast_start_tuple->value->int32 + (time_t) ((sun_event_words[0] & 0x7fff) * 60);
        sun_event_times[1] = (time_t) forecast_start_tuple->value->int32 + (time_t) (sun_event_words[1] * 60);
        persist_set_sun_event_start_type(sun_event_start_type);
        persist_set_sun_event_times(sun_event_times, 2);
        persist_set_precip_total((uint16_t) precip_total_tuple->value->int32);
        const uint8_t precip_type = precip_type_tuple->length == 1
            ? (uint8_t) precip_type_tuple->value->data[0]
            : (uint8_t) precip_type_tuple->value->int32;
        persist_set_precip_type(precip_type);
        loading_layer_refresh();
        forecast_layer_refresh();
        weather_status_layer_refresh();
        calendar_layer_refresh();
        calendar_status_layer_refresh();
    }
    else if (has_clay_config) {
        // Clay config data received
        Config config;
        persist_get_config(&config);

        if (clay_celsius_tuple) config.celsius = (bool) (clay_celsius_tuple->value->int16);
        if (clay_time_lead_zero_tuple) config.time_lead_zero = (bool) (clay_time_lead_zero_tuple->value->int16);
        if (clay_axis_12h_tuple) config.axis_12h = (bool) (clay_axis_12h_tuple->value->int16);
        if (clay_start_mon_tuple) config.start_mon = (bool) (clay_start_mon_tuple->value->int16);
        if (clay_prev_week_tuple) config.prev_week = (bool) (clay_prev_week_tuple->value->int16);
        if (clay_time_font_tuple) config.time_font = clay_time_font_tuple->value->int16;
        if (clay_color_today_tuple) config.color_today = GColorFromHEX(clay_color_today_tuple->value->int32);
        if (clay_vibe_tuple) config.vibe = (bool) (clay_vibe_tuple->value->int16);
        if (clay_show_qt_tuple) config.show_qt = (bool) (clay_show_qt_tuple->value->int16);
        if (clay_show_bt_tuple) config.show_bt = (bool) (clay_show_bt_tuple->value->int16);
        if (clay_show_bt_disconnect_tuple) config.show_bt_disconnect = (bool) (clay_show_bt_disconnect_tuple->value->int16);
        if (clay_show_am_pm_tuple) config.show_am_pm = (bool) (clay_show_am_pm_tuple->value->int16);
        if (clay_color_saturday_tuple) config.color_saturday = GColorFromHEX(clay_color_saturday_tuple->value->int32);
        if (clay_color_sunday_tuple) config.color_sunday = GColorFromHEX(clay_color_sunday_tuple->value->int32);
        if (clay_color_us_federal_tuple) config.color_us_federal = GColorFromHEX(clay_color_us_federal_tuple->value->int32);
        if (clay_color_time_tuple) config.color_time = GColorFromHEX(clay_color_time_tuple->value->int32);
        if (clay_day_night_shading_tuple) config.day_night_shading = (bool) (clay_day_night_shading_tuple->value->int16);
        if (clay_weather_status_right_mode_tuple) config.status_bar_mode = (enum StatusBarMode) (clay_weather_status_right_mode_tuple->value->int16);

        persist_set_config(config);
        main_window_refresh();
    }
    else {
        APP_LOG(APP_LOG_LEVEL_WARNING, "Bad payload received in app_message.c");
    }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped: %d", (int) reason);
}

void app_message_send_startup_state(bool has_forecast_data) {
    DictionaryIterator *outbox;
    AppMessageResult result = app_message_outbox_begin(&outbox);

    if (result != APP_MSG_OK) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Unable to begin startup outbox: %d", result);
        return;
    }

    dict_write_uint8(outbox, MESSAGE_KEY_WATCH_HAS_FORECAST_DATA, has_forecast_data ? 1 : 0);
    result = app_message_outbox_send();

    if (result != APP_MSG_OK) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Unable to send startup state: %d", result);
    }
}

void app_message_init() {
    // Register callbacks
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);

    // Open AppMessage
    const uint32_t inbox_size_max = app_message_inbox_size_maximum();
    uint32_t inbox_size_chosen = inbox_size_max;
    const int outbox_size = dict_calc_buffer_size(1, sizeof(uint8_t));
    AppMessageResult open_result = app_message_open(inbox_size_chosen, outbox_size);

    if (open_result == APP_MSG_OUT_OF_MEMORY && inbox_size_chosen > APP_MESSAGE_INBOX_SIZE_MINIMUM) {
        inbox_size_chosen = APP_MESSAGE_INBOX_SIZE_MINIMUM;
        open_result = app_message_open(inbox_size_chosen, outbox_size);
    }

    APP_LOG(APP_LOG_LEVEL_INFO, "AppMessage buffer sizes: inbox=%lu outbox=%d",
            (unsigned long) inbox_size_chosen, outbox_size);

    if (open_result != APP_MSG_OK) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "AppMessage open failed: %d", (int) open_result);
    } else {
        MEMORY_LOG_HEAP("after_app_message_open");
    }
}
