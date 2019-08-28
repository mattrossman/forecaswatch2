#include "weather_status_layer.h"
#include "c/appendix/persist.h"

#define FONT_18_OFFSET 7
#define CITY_MAX_WIDTH 100
#define ARROW_HEIGHT 8

static Layer *s_weather_status_layer;
static TextLayer *s_city_layer;
static TextLayer *s_current_temp_layer;
static TextLayer *s_next_sun_event_layer;

static GPath *s_arrow_path = NULL;
static const GPathInfo ARROW_PATH_INFO = {
    // Downward facing arrow
    .num_points = 5,
    .points = (GPoint[]){
        {0, -ARROW_HEIGHT/2}, {0, ARROW_HEIGHT/2}, {-3, ARROW_HEIGHT/2 - 3},
        {0, ARROW_HEIGHT/2}, {3, ARROW_HEIGHT/2 - 3}
    }
};

static void city_layer_refresh() {
    // Set the city text layer contents from storage
    static char s_city_buffer[20];
    persist_get_city(s_city_buffer, sizeof(s_city_buffer));
    text_layer_set_text(s_city_layer, s_city_buffer);
}

static void current_temp_layer_refresh() {
    static char s_temp_buffer[8];
    snprintf(s_temp_buffer, sizeof(s_temp_buffer), "• %d", persist_get_current_temp());
    text_layer_set_text(s_current_temp_layer, s_temp_buffer);
}

static void city_layer_init(GRect bounds) {
    // Set up the city text layer properties
    int w = bounds.size.w;
    int h = bounds.size.h;

    // Current temperature
    s_current_temp_layer = text_layer_create(GRect(2, -FONT_18_OFFSET, 40, 25));
    text_layer_set_background_color(s_current_temp_layer, GColorClear);
    text_layer_set_text_alignment(s_current_temp_layer, GTextAlignmentLeft);
    text_layer_set_text_color(s_current_temp_layer, GColorWhite);
    text_layer_set_font(s_current_temp_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));

    // City where weather was fetched
    s_city_layer = text_layer_create(GRect(w/2 - CITY_MAX_WIDTH/2, 4 - FONT_18_OFFSET, CITY_MAX_WIDTH, 25));
    text_layer_set_background_color(s_city_layer, GColorClear);
    text_layer_set_text_alignment(s_city_layer, GTextAlignmentCenter);
    text_layer_set_text_color(s_city_layer, GColorWhite);
    text_layer_set_font(s_city_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));

    // Time of next sun event (sunrise/sunset)
    s_next_sun_event_layer = text_layer_create(GRect(w - 2 - 6 - 40, 4 - FONT_18_OFFSET, 40, 25));
    text_layer_set_background_color(s_next_sun_event_layer, GColorClear);
    text_layer_set_text_alignment(s_next_sun_event_layer, GTextAlignmentRight);
    text_layer_set_text_color(s_next_sun_event_layer, GColorWhite);
    text_layer_set_font(s_next_sun_event_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text(s_next_sun_event_layer, "7:42");

    city_layer_refresh();
    current_temp_layer_refresh();
}

static void weather_status_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    int w = bounds.size.w;
    int h = bounds.size.h;
    // graphics_context_set_stroke_color(ctx, GColorRed);
    // graphics_draw_rect(ctx, bounds);
    s_arrow_path = gpath_create(&ARROW_PATH_INFO);
    // Translate by (5, 5):
    gpath_move_to(s_arrow_path, GPoint(w - 4, 6));
    graphics_context_set_stroke_color(ctx, GColorWhite);
    gpath_draw_outline_open(ctx, s_arrow_path);
    gpath_destroy(s_arrow_path);
}

void weather_status_layer_create(Layer* parent_layer, GRect frame) {
    s_weather_status_layer = layer_create(frame);
    GRect bounds = layer_get_bounds(s_weather_status_layer);

    // Set up the city name text layer
    city_layer_init(bounds);
    layer_add_child(s_weather_status_layer, text_layer_get_layer(s_city_layer));
    layer_add_child(s_weather_status_layer, text_layer_get_layer(s_current_temp_layer));
    layer_add_child(s_weather_status_layer, text_layer_get_layer(s_next_sun_event_layer));
    layer_set_update_proc(s_weather_status_layer, weather_status_update_proc);

    // Add the weather status bar to its parent
    layer_add_child(parent_layer, s_weather_status_layer);
}

void weather_status_layer_refresh() {
    layer_mark_dirty(s_weather_status_layer);
    city_layer_refresh();
    current_temp_layer_refresh();
}

void weather_status_layer_destroy() {
    text_layer_destroy(s_city_layer);
    text_layer_destroy(s_current_temp_layer);
    text_layer_destroy(s_next_sun_event_layer);
    layer_destroy(s_weather_status_layer);
}