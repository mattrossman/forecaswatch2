#include "weather_status_layer.h"
#include "c/appendix/persist.h"
#include "c/appendix/config.h"

#define FONT_18_OFFSET 7
#define FONT_14_OFFSET 3
#define CITY_INIT_WIDTH 100
#define ARROW_H 8
#define ARROW_HEAD_H 3
#define ARROW_HEAD_W 2
#define ARROW_W 6
#define MARGIN 2

static GRect frame_curr_temp;
static GRect frame_sun_event;

static Layer *s_weather_status_layer;
static TextLayer *s_city_layer;
static TextLayer *s_current_temp_layer;
static TextLayer *s_next_sun_event_layer;

static GPath *s_arrow_path = NULL;
static const GPathInfo ARROW_PATH_INFO = {
    // Downward facing arrow, centered at the origin
    .num_points = 6,
    .points = (GPoint[]){
        {0, -ARROW_H/2},
        {0, ARROW_H/2 - ARROW_HEAD_H},
        {-ARROW_HEAD_W, ARROW_H/2 - ARROW_HEAD_H},
        {0, ARROW_H/2},
        {ARROW_HEAD_W, ARROW_H/2 - ARROW_HEAD_H},
        {0, ARROW_H/2 - ARROW_HEAD_H}
    }
};

static void text_layer_move_frame(TextLayer *text_layer, GRect frame) {
    layer_set_frame(text_layer_get_layer(text_layer), frame);
}

static void city_layer_refresh() {
    // Set the city text layer contents from storage
    static char s_city_buffer[20];
    persist_get_city(s_city_buffer, sizeof(s_city_buffer));
    text_layer_set_text(s_city_layer, s_city_buffer);
}

static void current_temp_layer_refresh() {
    static char s_temp_buffer[8];
    snprintf(s_temp_buffer, sizeof(s_temp_buffer), "• %d", config_localize_temp(persist_get_current_temp()));
    text_layer_set_text(s_current_temp_layer, s_temp_buffer);

    // Dynamic resizing
    text_layer_move_frame(s_current_temp_layer, GRect(0, 0, 100, 100));  // Make it big so content doesn't get clipped
    GSize size = text_layer_get_content_size(s_current_temp_layer);
    text_layer_move_frame(s_current_temp_layer, GRect(MARGIN, -FONT_18_OFFSET, size.w, size.h));
    frame_curr_temp = GRect(0, -FONT_18_OFFSET, size.w + MARGIN, size.h);
}

static void sun_event_layer_refresh() {
    GRect bounds = layer_get_bounds(s_weather_status_layer);
    // Get the time of the first sun event
    time_t first_sun_event_time;
    persist_get_sun_event_times(&first_sun_event_time, 1);
    struct tm *sun_time = localtime(&first_sun_event_time);

    static char s_buffer[8];
    config_format_time(s_buffer, 8, sun_time);

    // Display this time on the TextLayer
    text_layer_set_text(s_next_sun_event_layer, s_buffer);
    text_layer_set_text(s_next_sun_event_layer, "17:42");

    // Dynamic resizing
    text_layer_move_frame(s_next_sun_event_layer, GRect(0, 0, 100, 100));  // Make it big so content doesn't get clipped
    GSize size = text_layer_get_content_size(s_next_sun_event_layer);
    text_layer_move_frame(s_next_sun_event_layer,
        GRect(bounds.size.w - MARGIN - ARROW_W - size.w, -FONT_14_OFFSET, size.w + ARROW_W, size.h));
    frame_sun_event = GRect(bounds.size.w - MARGIN - ARROW_W - size.w, -FONT_14_OFFSET, size.w + ARROW_W + MARGIN, size.h);
}

static void weather_status_layer_init(GRect bounds) {
    // Set up the city text layer properties
    int w = bounds.size.w;
    int h = bounds.size.h;

    // Current temperature
    s_current_temp_layer = text_layer_create(GRect(MARGIN, -FONT_18_OFFSET, 40, 25));
    text_layer_set_background_color(s_current_temp_layer, GColorClear);
    text_layer_set_text_alignment(s_current_temp_layer, GTextAlignmentLeft);
    text_layer_set_text_color(s_current_temp_layer, GColorWhite);
    text_layer_set_font(s_current_temp_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_background_color(s_current_temp_layer, GColorRed);

    // City where weather was fetched
    s_city_layer = text_layer_create(GRect(w/2 - CITY_INIT_WIDTH/2, 4 - FONT_18_OFFSET, CITY_INIT_WIDTH, 25));
    text_layer_set_background_color(s_city_layer, GColorClear);
    text_layer_set_text_alignment(s_city_layer, GTextAlignmentCenter);
    text_layer_set_text_color(s_city_layer, GColorWhite);
    text_layer_set_font(s_city_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));

    // Time of next sun event (sunrise/sunset)
    s_next_sun_event_layer = text_layer_create(GRect(w - MARGIN - 6 - 40, 4 - FONT_18_OFFSET, 40, 25));
    text_layer_set_background_color(s_next_sun_event_layer, GColorClear);
    text_layer_set_text_alignment(s_next_sun_event_layer, GTextAlignmentLeft);
    text_layer_set_text_color(s_next_sun_event_layer, GColorWhite);
    text_layer_set_font(s_next_sun_event_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    // text_layer_set_background_color(s_next_sun_event_layer, GColorGreen);
    // text_layer_set_text(s_next_sun_event_layer, "7:42");

    current_temp_layer_refresh();
    sun_event_layer_refresh();
    city_layer_refresh();
}

static void weather_status_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    int w = bounds.size.w;
    int h = bounds.size.h;
    s_arrow_path = gpath_create(&ARROW_PATH_INFO);
    // Translate to correct location in layer
    if (persist_get_sun_event_start_type() == 0)
        gpath_rotate_to(s_arrow_path, TRIG_MAX_ANGLE / 2);
    gpath_move_to(s_arrow_path, GPoint(w - 4, 6));
    graphics_context_set_stroke_color(ctx, GColorWhite);
    gpath_draw_outline_open(ctx, s_arrow_path);
    graphics_context_set_fill_color(ctx, GColorWhite);
    gpath_draw_filled(ctx, s_arrow_path);
    gpath_destroy(s_arrow_path);
}

void weather_status_layer_create(Layer* parent_layer, GRect frame) {
    s_weather_status_layer = layer_create(frame);
    GRect bounds = layer_get_bounds(s_weather_status_layer);

    // Set up all the text layers
    weather_status_layer_init(bounds);
    layer_add_child(s_weather_status_layer, text_layer_get_layer(s_city_layer));
    layer_add_child(s_weather_status_layer, text_layer_get_layer(s_current_temp_layer));
    layer_add_child(s_weather_status_layer, text_layer_get_layer(s_next_sun_event_layer));
    layer_set_update_proc(s_weather_status_layer, weather_status_update_proc);

    // Add the weather status bar to its parent
    layer_add_child(parent_layer, s_weather_status_layer);
}

void weather_status_layer_refresh() {
    layer_mark_dirty(s_weather_status_layer);
    current_temp_layer_refresh();
    sun_event_layer_refresh();
    city_layer_refresh();
}

void weather_status_layer_destroy() {
    text_layer_destroy(s_city_layer);
    text_layer_destroy(s_current_temp_layer);
    text_layer_destroy(s_next_sun_event_layer);
    layer_destroy(s_weather_status_layer);
}