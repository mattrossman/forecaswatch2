#include "battery_layer.h"
#include "c/appendix/persist.h"
#include "c/appendix/memory_log.h"
#include "c/appendix/battery_indicator.h"
#include "c/services/watch_services.h"

static Layer *s_battery_layer;
static bool s_battery_subscribed;

static void battery_state_handler(BatteryChargeState charge) {
    battery_layer_refresh();
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    battery_indicator_draw(ctx, bounds);
}

void battery_layer_create(Layer* parent_layer, GRect frame) {
    MemoryHeapProbe probe = MEMORY_HEAP_PROBE_START("battery_layer_create");

    s_battery_layer = layer_create(frame);
    MEMORY_HEAP_PROBE_SAMPLE("after_layer_create", &probe);

    layer_set_update_proc(s_battery_layer, battery_update_proc);
    if (!watch_services_battery_is_fixture()) {
        battery_state_service_subscribe(battery_state_handler);
        s_battery_subscribed = true;
    } else {
        s_battery_subscribed = false;
    }
    MEMORY_HEAP_PROBE_SAMPLE("after_battery_subscribe", &probe);
    layer_add_child(parent_layer, s_battery_layer);
    MEMORY_HEAP_PROBE_SAMPLE("after_layer_add_child", &probe);
    MEMORY_LOG_HEAP("after_battery_layer_create");
    MEMORY_HEAP_PROBE_LOG_MIN(&probe);
}

void battery_layer_refresh() {
    layer_mark_dirty(s_battery_layer);
}

void battery_layer_destroy() {
    MEMORY_LOG_HEAP("battery_layer_destroy:before");
    if (s_battery_subscribed) {
        battery_state_service_unsubscribe();
        s_battery_subscribed = false;
    }
    battery_indicator_unload();
    layer_destroy(s_battery_layer);
    MEMORY_LOG_HEAP("battery_layer_destroy:after");
}
