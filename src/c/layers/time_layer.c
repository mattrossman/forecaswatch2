#include "time_layer.h"
#include "c/appendix/config.h"

static Layer *s_container_layer;
static TextLayer *s_time_layer;

void time_layer_create(Layer* parent_layer, GRect frame) {
    s_container_layer = layer_create(frame);
    s_time_layer = text_layer_create(GRect(0, 0, frame.size.w, frame.size.h));
    // Improve the layout to be more like a watchface
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text_color(s_time_layer, GColorWhite);
    text_layer_set_text(s_time_layer, "00:00");
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

    // Add it as a child layer to the Window's root layer
    layer_add_child(s_container_layer, text_layer_get_layer(s_time_layer));
    layer_add_child(parent_layer, s_container_layer);

}

// 12:30 -> 12:30
// 13:30 -> 1:30
// 00:30 -> 12:30

static void text_layer_move_frame(TextLayer *text_layer, GRect frame) {
    layer_set_frame(text_layer_get_layer(text_layer), frame);
}

void time_layer_refresh() {
    // Set up font and its positioning
    text_layer_set_font(s_time_layer, config_time_font());
    GSize time_size = text_layer_get_content_size(s_time_layer);
    GRect bounds = layer_get_bounds(s_container_layer);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Content height: %d", time_size.h);
    text_layer_move_frame(s_time_layer, GRect(0, (49 - time_size.h) / 2, bounds.size.w, bounds.size.h));

    // Get a tm structure
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

    // Format the time into a buffer
    static char s_buffer[8];
    config_format_time(s_buffer, 8, tick_time);

    // Display this time on the TextLayer
    text_layer_set_text(s_time_layer, s_buffer);
}

void time_layer_destroy() {
    text_layer_destroy(s_time_layer);
}