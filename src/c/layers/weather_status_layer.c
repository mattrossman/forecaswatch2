#include "weather_status_layer.h"
#include "c/appendix/persist.h"

#define FONT_18_OFFSET 7
#define CITY_BUFFER_SIZE 20
#define CITY_MAX_WIDTH 100

static Layer *s_weather_status_layer;
static TextLayer *s_city_layer;
char city_buffer[CITY_BUFFER_SIZE];

static void city_layer_refresh() {
    // Set the city text layer contents from storage
    persist_get_city(city_buffer, CITY_BUFFER_SIZE);
    text_layer_set_text(s_city_layer, city_buffer);
}

static void city_layer_init(GRect bounds) {
    // Set up the city text layer properties
    int w = bounds.size.w;
    int h = bounds.size.h;
    s_city_layer = text_layer_create(GRect(w/2 - CITY_MAX_WIDTH/2, -FONT_18_OFFSET, CITY_MAX_WIDTH,25));
    text_layer_set_background_color(s_city_layer, GColorClear);
    text_layer_set_text_alignment(s_city_layer, GTextAlignmentCenter);
    text_layer_set_text_color(s_city_layer, GColorWhite);
    text_layer_set_font(s_city_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    city_layer_refresh();
}

static void weather_status_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    graphics_context_set_stroke_color(ctx, GColorRed);
    graphics_draw_rect(ctx, bounds);
}

void weather_status_layer_create(Layer* parent_layer, GRect frame) {
    s_weather_status_layer = layer_create(frame);
    GRect bounds = layer_get_bounds(s_weather_status_layer);

    // Set up the city name text layer
    city_layer_init(bounds);
    layer_add_child(s_weather_status_layer, text_layer_get_layer(s_city_layer));
    layer_set_update_proc(s_weather_status_layer, weather_status_update_proc);

    // Add the weather status bar to its parent
    layer_add_child(parent_layer, s_weather_status_layer);
}

void weather_status_layer_refresh() {
    layer_mark_dirty(s_weather_status_layer);
    city_layer_refresh();
}

void weather_status_layer_destroy() {
    layer_destroy(s_weather_status_layer);
}