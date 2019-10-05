#include "time_layer.h"

static TextLayer *s_time_layer;

void time_layer_create(Layer* parent_layer, GRect frame) {
    s_time_layer = text_layer_create(frame);
    // Improve the layout to be more like a watchface
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text_color(s_time_layer, GColorWhite);
    text_layer_set_text(s_time_layer, "00:00");
    text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

    // Add it as a child layer to the Window's root layer
    layer_add_child(parent_layer, text_layer_get_layer(s_time_layer));

}

// 12:30 -> 12:30
// 13:30 -> 1:30
// 00:30 -> 12:30

void time_layer_refresh() {
    // Get a tm structure
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

    // Format the time into a buffer
    static char s_buffer[8];
    strftime(s_buffer, 8, clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);

    // Display this time on the TextLayer
    text_layer_set_text(s_time_layer, s_buffer);
}

void time_layer_destroy() {
    text_layer_destroy(s_time_layer);
}