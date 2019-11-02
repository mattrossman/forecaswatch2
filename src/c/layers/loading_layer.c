#include "loading_layer.h"
#include "c/appendix/persist.h"

static Layer *s_loading_layer;
static TextLayer *s_loading_text_layer;

static void loading_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    int w = bounds.size.w;
    int h = bounds.size.h;

    // Black out the weather components
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(0, 0, w, h), 0, GCornerNone);
}

void loading_layer_create(Layer* parent_layer, GRect frame) {
    s_loading_layer = layer_create(frame);

    GRect bounds = layer_get_bounds(s_loading_layer);
    int w = bounds.size.w; int h = bounds.size.h;
    s_loading_text_layer = text_layer_create(GRect(0, h / 3, w, h));
    text_layer_set_background_color(s_loading_text_layer, GColorClear);
    text_layer_set_text_color(s_loading_text_layer, GColorWhite);
    text_layer_set_font(s_loading_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text_alignment(s_loading_text_layer, GTextAlignmentCenter);
    text_layer_set_text(s_loading_text_layer, "No data :(");

    layer_set_update_proc(s_loading_layer, loading_update_proc);
    layer_add_child(s_loading_layer, text_layer_get_layer(s_loading_text_layer));
    layer_add_child(parent_layer, s_loading_layer);
}

void loading_layer_refresh() {
    const time_t forecast_start = persist_get_forecast_start();
    const time_t now = time(NULL);
    if (now - forecast_start > 60 * 60 * 12) // 60 sec/min * 60 min/h * 12h
        layer_set_hidden(s_loading_layer, false); // show the no data notice
    else
        layer_set_hidden(s_loading_layer, true); // hide the no data notice
}

void loading_layer_destroy() {
    layer_destroy(s_loading_layer);
    text_layer_destroy(s_loading_text_layer);
}