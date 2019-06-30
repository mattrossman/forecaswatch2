#include "battery_layer.h"

#define BATTERY_HEIGHT 10
#define BATTERY_NUB_SIZE 2  // Size of the nub on the right side of the battery


static Layer *s_battery_layer;

static void battery_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    int w = bounds.size.w;
    int h = bounds.size.h;

    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_rect(ctx, GRect(0,0,w,BATTERY_HEIGHT));
}

void battery_layer_create(Layer* parent_layer, GRect frame) {
    s_battery_layer = layer_create(frame);
    layer_set_update_proc(s_battery_layer, battery_update_proc);
    layer_add_child(parent_layer, s_battery_layer);
}

void battery_layer_refresh() {
    layer_mark_dirty(s_battery_layer);
}

void battery_layer_destroy() {
    layer_destroy(s_battery_layer);
}