#include "graph_bottom_layer.h"

static Layer *s_graph_bottom_layer;
const int s_forecast_start_hour = 6;
static TextLayer *s_axis_label_layers[6];
static char s_hour_buf[6][4];

void graph_bottom_layer_create(Layer* parent_layer, GRect frame) {
    s_graph_bottom_layer = layer_create(frame);
    GRect bounds = layer_get_bounds(s_graph_bottom_layer);
    const int s_entry_width = 20;
    for (int i = 0; i < 6; ++i) {
        TextLayer *text_layer = text_layer_create(GRect(i * s_entry_width, 0, s_entry_width, 20));
        s_axis_label_layers[i] = text_layer;
        int hour = s_forecast_start_hour + i * 2;
        text_layer_set_background_color(text_layer, GColorClear);
        text_layer_set_text_color(text_layer, GColorWhite);
        text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
        text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
        snprintf(s_hour_buf[i], sizeof(s_hour_buf[i]), "%d", hour);
        APP_LOG(APP_LOG_LEVEL_DEBUG, "(%d) hour: %d", i, hour);
        APP_LOG(APP_LOG_LEVEL_DEBUG, "(%d) buffer: %s", i, s_hour_buf[i]);

        text_layer_set_text(text_layer, s_hour_buf[i]);
        layer_add_child(s_graph_bottom_layer, text_layer_get_layer(text_layer));
    }
    
    for (int i = 0; i < 6; ++i) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "(%d) contents: %s", i, text_layer_get_text(s_axis_label_layers[i]));
    }
    layer_add_child(parent_layer, s_graph_bottom_layer);
}

void graph_bottom_layer_refresh() {

}

void graph_bottom_layer_destroy() {
    layer_destroy(s_graph_bottom_layer);
    for (int i = 0; i < 6; ++i) {
        text_layer_destroy(s_axis_label_layers[i]);
    }
}