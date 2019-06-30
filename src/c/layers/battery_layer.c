#include "battery_layer.h"
#include "c/appendix/persist.h"

#define BATTERY_HEIGHT 10
#define BATTERY_NUB_SIZE 2  // Size of the nub on the right side of the battery


static Layer *s_battery_layer;

static void battery_state_handler(BatteryChargeState charge) {
    persist_set_battery_level(charge.charge_percent);
    battery_layer_refresh();
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    int w = bounds.size.w;
    int h = bounds.size.h;

    float battery_w = w * (float) persist_get_battery_level() / 100.0;
    graphics_context_set_fill_color(ctx, GColorGreen);
    graphics_fill_rect(ctx, GRect(0, 0, battery_w, BATTERY_HEIGHT), 0, GCornerNone);
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_rect(ctx, GRect(0, 0, w, BATTERY_HEIGHT));
}

void battery_layer_create(Layer* parent_layer, GRect frame) {
    s_battery_layer = layer_create(frame);
    layer_set_update_proc(s_battery_layer, battery_update_proc);
    battery_state_service_subscribe(battery_state_handler);
    layer_add_child(parent_layer, s_battery_layer);
}

void battery_layer_refresh() {
    layer_mark_dirty(s_battery_layer);
}

void battery_layer_destroy() {
    battery_state_service_unsubscribe();
    layer_destroy(s_battery_layer);
}