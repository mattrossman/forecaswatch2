#include "graph_bottom_layer.h"

static Layer *s_graph_bottom_layer;
const int s_forecast_start_hour = 6;
static TextLayer *s_axis_label_layers[6];
static char s_hour_buf[6][4];

static void graph_bottom_update_proc(Layer *layer, GContext *ctx) {
    // Weather section outline
    GRect bounds = layer_get_bounds(layer);
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_line(ctx, GPointZero, GPoint(bounds.size.w, 0));
}

void graph_bottom_layer_create(Layer* parent_layer, GRect frame) {
    s_graph_bottom_layer = layer_create(frame);
    GRect bounds = layer_get_bounds(s_graph_bottom_layer);

    // Set up contents
    layer_set_update_proc(s_graph_bottom_layer, graph_bottom_update_proc);

    const int s_entry_width = 10;
    for (int i = 0; i < 6; ++i) {
        TextLayer *text_layer = text_layer_create(GRect(i * 2 * s_entry_width, -3, s_entry_width * 2, 20));
        s_axis_label_layers[i] = text_layer;

        // Hour labels formatting
        text_layer_set_background_color(text_layer, GColorClear);
        text_layer_set_text_color(text_layer, GColorWhite);
        text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
        text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);


        // Write the (int) hour to a character buffer and point the TextLayer at it
        int hour = s_forecast_start_hour + i * 2;
        snprintf(s_hour_buf[i], sizeof(s_hour_buf[i]), "%d", hour);
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