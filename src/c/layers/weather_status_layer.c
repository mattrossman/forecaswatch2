#include "weather_status_layer.h"
#include "c/appendix/persist.h"

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
    s_city_layer = text_layer_create(GRect(w/2 - CITY_MAX_WIDTH/2, 0, CITY_MAX_WIDTH,20));
    text_layer_set_background_color(s_city_layer, GColorClear);
    text_layer_set_text_alignment(s_city_layer, GTextAlignmentCenter);
    text_layer_set_text_color(s_city_layer, GColorWhite);
    text_layer_set_font(s_city_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    city_layer_refresh();
}

void weather_status_layer_create(Layer* parent_layer, GRect frame) {
    s_weather_status_layer = layer_create(frame);
    GRect bounds = layer_get_bounds(s_weather_status_layer);

    // Set up the city name text layer
    city_layer_init(bounds);
    layer_add_child(s_weather_status_layer, text_layer_get_layer(s_city_layer));

    // Add the weather status bar to its parent
    layer_add_child(parent_layer, s_weather_status_layer);
}

void weather_status_layer_refresh() {
    city_layer_refresh();
}

void weather_status_layer_destroy() {
    layer_destroy(s_weather_status_layer);
}