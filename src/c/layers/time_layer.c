#include "time_layer.h"
#include "c/appendix/config.h"

// MT = Margin Top
#define MT_TIME 14
#define MT_AM_PM 7


static TextLayer *s_container_layer;
static TextLayer *s_time_layer;
static TextLayer *s_am_pm_layer;

void time_layer_create(Layer* parent_layer, GRect frame) {
    s_container_layer = text_layer_create(frame);
    s_time_layer = text_layer_create(GRect(0, 0, frame.size.w, frame.size.h));
    s_am_pm_layer = text_layer_create(GRect(0, 0, 30, frame.size.h));

    text_layer_set_background_color(s_container_layer, GColorClear);

    // Main time formatting
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text_color(s_time_layer, GColorWhite);
    text_layer_set_text(s_time_layer, "00:00");
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentLeft);

    // AM/PM formatting
    text_layer_set_font(s_am_pm_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_background_color(s_am_pm_layer, GColorClear);
    text_layer_set_text_color(s_am_pm_layer, GColorWhite);
    text_layer_set_text(s_am_pm_layer, "PM");
    text_layer_set_text_alignment(s_am_pm_layer, GTextAlignmentLeft);

    layer_add_child(text_layer_get_layer(s_container_layer), text_layer_get_layer(s_time_layer));
    layer_add_child(text_layer_get_layer(s_time_layer), text_layer_get_layer(s_am_pm_layer));
    layer_add_child(parent_layer, text_layer_get_layer(s_container_layer));

}

// 12:30 -> 12:30
// 13:30 -> 1:30
// 00:30 -> 12:30

static void text_layer_move_frame(TextLayer *text_layer, GRect frame) {
    layer_set_frame(text_layer_get_layer(text_layer), frame);
}

void time_layer_tick() {
    // Get a tm structure
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

    // Format the time into a buffer
    static char s_buffer[8];
    config_format_time(s_buffer, 8, tick_time);

    // Update the time and AM/PM indicator
    text_layer_set_text(s_time_layer, s_buffer);
    if (g_config->show_am_pm)
        text_layer_set_text(s_am_pm_layer, tick_time->tm_hour < 12 ? "AM" : "PM");
}

void time_layer_refresh() {
    time_layer_tick();  // Update time text

    // Set up font and its positioning
    text_layer_set_font(s_time_layer, config_time_font());
    GRect bounds = layer_get_bounds(text_layer_get_layer(s_container_layer));
    text_layer_move_frame(s_time_layer, GRect(0, 0, bounds.size.w, bounds.size.h)); // Reset for size calculation
    GSize time_size = text_layer_get_content_size(s_time_layer);
    GSize am_pm_size = text_layer_get_content_size(s_am_pm_layer);

    // Calculate some landmarks
    int content_w = time_size.w + (g_config->show_am_pm ? am_pm_size.w : 0);
    int text_h = time_size.h - MT_TIME; // Remove top margin, approximately
    int text_top = -MT_TIME + (bounds.size.h/2 - text_h/2);
    int text_left = bounds.size.w / 2 - content_w / 2;

    // Update layer positions and visibility
    text_layer_move_frame(s_time_layer, GRect(text_left, text_top, content_w, time_size.h));
    if (g_config->show_am_pm)
        text_layer_move_frame(s_am_pm_layer, GRect(time_size.w, MT_TIME - MT_AM_PM, 30, time_size.h));
    layer_set_hidden(text_layer_get_layer(s_am_pm_layer), !g_config->show_am_pm);
}

void time_layer_destroy() {
    text_layer_destroy(s_time_layer);
    text_layer_destroy(s_container_layer);
}