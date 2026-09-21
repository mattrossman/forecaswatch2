#include "app_message.h"
#include "persist.h"
#include "math.h"
#include "c/layers/forecast_layer.h"
#include "c/layers/weather_status_layer.h"
#include "c/layers/loading_layer.h"
#include "c/layers/calendar_layer.h"
#include "c/layers/calendar_status_layer.h"
#include "c/appendix/stats_metrics.h"
#include "c/layers/stats_layer.h"
#include "c/windows/main_window.h"
#include "memory_log.h"
#include <string.h>

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Message received!");
    // Weather data
    Tuple *temp_trend_tuple = dict_find(iterator, MESSAGE_KEY_TEMP_TREND_INT16);
    Tuple *precip_trend_tuple = dict_find(iterator, MESSAGE_KEY_PRECIP_TREND_UINT8);
    Tuple *precip_amount_trend_tuple = dict_find(iterator, MESSAGE_KEY_PRECIP_AMOUNT_TREND_UINT8);
    Tuple *forecast_start_tuple = dict_find(iterator, MESSAGE_KEY_FORECAST_START);
    Tuple *num_entries_tuple = dict_find(iterator, MESSAGE_KEY_NUM_ENTRIES);
    Tuple *current_temp_tuple = dict_find(iterator, MESSAGE_KEY_CURRENT_TEMP);
    Tuple *city_tuple = dict_find(iterator, MESSAGE_KEY_CITY);
    Tuple *sun_events_tuple = dict_find(iterator, MESSAGE_KEY_SUN_EVENTS);
    /* Optional, and deliberately absent from the guard below: an older phone
       bundle without UV must still deliver its weather. */
    Tuple *uv_index_tuple = dict_find(iterator, MESSAGE_KEY_UV_INDEX);

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
    Tuple *clay_precip_amount_bars_tuple = dict_find(iterator, MESSAGE_KEY_CLAY_PRECIP_AMOUNT_BARS);
    /* Read optionally, never added to the guard below: that condition is
       all-or-nothing, so a phone that omits this key would otherwise drop every
       setting. All slots ride in one byte array because the 256-byte inbox has
       no room for seven more tuples. */
    Tuple *clay_stats_slots_tuple = dict_find(iterator, MESSAGE_KEY_CLAY_STATS_SLOTS);

    if(temp_trend_tuple && precip_trend_tuple && precip_amount_trend_tuple && forecast_start_tuple && num_entries_tuple && city_tuple && sun_events_tuple) {
        // Weather data received
        APP_LOG(APP_LOG_LEVEL_INFO, "All tuples received!");
        persist_set_forecast_start((time_t)forecast_start_tuple->value->int32);
        const int num_entries = ((int)num_entries_tuple->value->int32);
        persist_set_num_entries(num_entries);
#ifdef FCW2_ENABLE_MEMORY_LOGGING
        APP_LOG(APP_LOG_LEVEL_DEBUG, "MEM|forecast_payload|entries=%d|free=%lu|used=%lu",
                num_entries,
                (unsigned long)heap_bytes_free(),
                (unsigned long)heap_bytes_used());
#endif
        int16_t *temp_data = (int16_t*) temp_trend_tuple->value->data;
        persist_set_temp_trend(temp_data, num_entries);
        uint8_t *precip_data = (uint8_t*) precip_trend_tuple->value->data;
        persist_set_precip_trend(precip_data, num_entries);
        persist_set_precip_amount_trend((uint8_t*) precip_amount_trend_tuple->value->data, num_entries);
        persist_set_city((char*)city_tuple->value->cstring);
        int lo, hi;
        min_max(temp_data, num_entries, &lo, &hi);
        persist_set_temp_lo(lo);
        persist_set_temp_hi(hi);
        persist_set_current_temp((int)current_temp_tuple->value->int32);
        persist_set_uv_index(uv_index_tuple ? (int) uv_index_tuple->value->int32 : -1);
        uint8_t sun_event_start_type = (uint8_t) sun_events_tuple->value->uint8;
        time_t *sun_event_times = (time_t*) (sun_events_tuple->value->data + 1);
        persist_set_sun_event_start_type(sun_event_start_type);
        persist_set_sun_event_times(sun_event_times, 2);
        loading_layer_refresh();
        forecast_layer_refresh();
        weather_status_layer_refresh();
        calendar_layer_refresh();
        calendar_status_layer_refresh();
    }
    else if (clay_celsius_tuple && clay_time_lead_zero_tuple && clay_axis_12h_tuple && clay_start_mon_tuple && clay_prev_week_tuple
        && clay_color_today_tuple && clay_time_font_tuple && clay_vibe_tuple && clay_show_qt_tuple && clay_show_bt_tuple
        && clay_show_bt_disconnect_tuple && clay_show_am_pm_tuple && clay_color_saturday_tuple && clay_color_sunday_tuple
        && clay_color_us_federal_tuple && clay_color_time_tuple && clay_day_night_shading_tuple && clay_precip_amount_bars_tuple) {
        // Clay config data received
        bool clay_celsius = (bool) (clay_celsius_tuple->value->int16);
        bool time_lead_zero = (bool) (clay_time_lead_zero_tuple->value->int16);
        bool axis_12h = (bool) (clay_axis_12h_tuple->value->int16);
        bool start_mon = (bool) (clay_start_mon_tuple->value->int16);
        bool prev_week = (bool) (clay_prev_week_tuple->value->int16);
        bool vibe = (bool) (clay_vibe_tuple->value->int16);
        bool show_qt = (bool) (clay_show_qt_tuple->value->int16);
        bool show_bt = (bool) (clay_show_bt_tuple->value->int16);
        bool show_bt_disconnect = (bool) (clay_show_bt_disconnect_tuple->value->int16);
        bool show_am_pm = (bool) (clay_show_am_pm_tuple->value->int16);
        bool day_night_shading = (bool) (clay_day_night_shading_tuple->value->int16);
        bool precip_amount_bars = (bool) (clay_precip_amount_bars_tuple->value->int16);
        int16_t time_font = clay_time_font_tuple->value->int16;
        GColor color_today = GColorFromHEX(clay_color_today_tuple->value->int32);
        GColor color_saturday = GColorFromHEX(clay_color_saturday_tuple->value->int32);
        GColor color_sunday = GColorFromHEX(clay_color_sunday_tuple->value->int32);
        GColor color_us_federal = GColorFromHEX(clay_color_us_federal_tuple->value->int32);
        GColor color_time = GColorFromHEX(clay_color_time_tuple->value->int32);

        /* Fall back to what is already configured, not to zeros: an older phone
           bundle that omits the tuple must leave the user's tiles intact rather
           than blanking them. Byte 0 is the view mode, bytes 1..n the slots. */
        bool top_band_stats = g_config->top_band_stats;
        uint8_t stat_slots[STATS_MAX_SLOTS];
        memcpy(stat_slots, g_config->stat_slots, sizeof(stat_slots));
        if (clay_stats_slots_tuple && clay_stats_slots_tuple->length >= 1) {
            const uint8_t *slot_data = clay_stats_slots_tuple->value->data;
            const uint16_t slot_len = clay_stats_slots_tuple->length;
            top_band_stats = (slot_data[0] == 1);
            for (int i = 0; i < STATS_MAX_SLOTS && (i + 1) < slot_len; ++i) {
                stat_slots[i] = (uint8_t) stats_metric_validate_id(slot_data[i + 1]);
            }
        }

        Config config = (Config) {
            .celsius = clay_celsius,
            .time_lead_zero = time_lead_zero,
            .axis_12h = axis_12h,
            .start_mon = start_mon,
            .prev_week = prev_week,
            .time_font = time_font,
            .color_today = color_today,
            .vibe = vibe,
            .show_qt = show_qt,
            .show_bt = show_bt,
            .show_bt_disconnect = show_bt_disconnect,
            .show_am_pm = show_am_pm,
            .color_saturday = color_saturday,
            .color_sunday = color_sunday,
            .color_us_federal = color_us_federal,
            .color_time = color_time,
            .day_night_shading = day_night_shading,
            .precip_amount_bars = precip_amount_bars,
            .top_band_stats = top_band_stats
        };
        memcpy(config.stat_slots, stat_slots, sizeof(config.stat_slots));
        persist_set_config(config);
        main_window_refresh();
        /* The phone is demonstrably alive, so this send will not be dropped. */
        app_message_send_stats_caps();
    }
    else {
        APP_LOG(APP_LOG_LEVEL_WARNING, "Bad payload received in app_message.c");
    }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

/* Which metrics this watch can supply, and how many slots its screen has room
   for, so the settings page only offers real options. */
static void write_stats_caps(DictionaryIterator *outbox) {
    const uint32_t caps = stats_metrics_caps();
    dict_write_int(outbox, MESSAGE_KEY_WATCH_STATS_CAPS, &caps, sizeof(caps), false);
    dict_write_uint8(outbox, MESSAGE_KEY_WATCH_STATS_SLOT_COUNT,
                     stats_layer_enabled() ? STATS_SLOT_COUNT : 0);
}

void app_message_send_stats_caps(void) {
    DictionaryIterator *outbox;
    if (app_message_outbox_begin(&outbox) != APP_MSG_OK) {
        return;
    }

    write_stats_caps(outbox);

    const AppMessageResult result = app_message_outbox_send();
    if (result != APP_MSG_OK) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Unable to send stats caps: %d", result);
    }
}

void app_message_send_startup_state(bool has_forecast_data) {
    DictionaryIterator *outbox;
    AppMessageResult result = app_message_outbox_begin(&outbox);

    if (result != APP_MSG_OK) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Unable to begin startup outbox: %d", result);
        return;
    }

    dict_write_uint8(outbox, MESSAGE_KEY_WATCH_HAS_FORECAST_DATA, has_forecast_data ? 1 : 0);
    write_stats_caps(outbox);
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
    const int inbox_size = 256;
    /* Three tuples now; sizing for one silently truncates the extra writes. */
    const int outbox_size = dict_calc_buffer_size(3, sizeof(uint8_t), sizeof(uint32_t), sizeof(uint8_t));
    APP_LOG(APP_LOG_LEVEL_INFO, "AppMessage buffer sizes: inbox=%d outbox=%d", inbox_size, outbox_size);
    app_message_open(inbox_size, outbox_size);
    MEMORY_LOG_HEAP("after_app_message_open");
}
