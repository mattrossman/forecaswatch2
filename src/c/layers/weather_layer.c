#include "weather_layer.h"
#include "graph_bottom_layer.h"

static Layer *s_weather_layer;
static Layer *s_graph_layer;
static TextLayer *s_hi_layer;
static TextLayer *s_lo_layer;

static void weather_layer_draw(Layer *layer, GContext *ctx) {
    // Weather section outline
    GRect bounds = layer_get_bounds(layer);
    graphics_context_set_stroke_color(ctx, GColorBlue);
    graphics_draw_rect(ctx, bounds);
}

void weather_layer_create(Layer *parent_layer, GRect frame) {
    s_weather_layer = layer_create(frame);
    GRect bounds = layer_get_bounds(s_weather_layer);

    // Temperature HIGH
    s_hi_layer = text_layer_create(GRect(0, 0, 20, 20));
    text_layer_set_background_color(s_hi_layer, GColorBlue);
    text_layer_set_text_alignment(s_hi_layer, GTextAlignmentCenter);
    text_layer_set_text_color(s_hi_layer, GColorWhite);
    text_layer_set_font(s_hi_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text(s_hi_layer, "12");
    layer_add_child(s_weather_layer, text_layer_get_layer(s_hi_layer));

    // Set up contents
    layer_set_update_proc(s_weather_layer, weather_layer_draw);

    graph_bottom_layer_create(s_weather_layer, GRect(20, bounds.size.h - 15, bounds.size.w-20, 15));

    // Add it as a child layer to the Window's root layer
    layer_add_child(parent_layer, s_weather_layer);
}

void weather_layer_refresh() {

}

void weather_layer_destroy() {
    text_layer_destroy(s_hi_layer);
    graph_bottom_layer_destroy();
    layer_destroy(s_weather_layer);
}