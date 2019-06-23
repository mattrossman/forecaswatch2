#include "graph_data_layer.h"
#include "c/appendix/constants.h"
#include "c/appendix/math.h"

static Layer *s_graph_data_layer;

const int bottom_axis_h = 15;
const int margin_temp_w = 10;
const int margin_temp_h = 10;

static void graph_data_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    int w = bounds.size.w;
    int h = bounds.size.h;

    // Graph section outline
    graphics_context_set_stroke_color(ctx, GColorRed);
    graphics_draw_rect(ctx, bounds);

    // Data setup
    int *data = c_12h_test_data;
    int lo, hi;
    min_max(data, 12, &lo, &hi);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "max: %d, min: %d", lo, hi);
    int range = hi - lo;

    // Draw a bounding box for each data entry
    float entry_w = (float) (bounds.size.w - 2 * margin_temp_w) / (c_num_graph_hours - 1);
    graphics_context_set_fill_color(ctx, GColorYellow);
    for (int i = 0; i < c_num_graph_hours; ++i) {
        int temp = data[i];
        int temp_h = (float) (temp - lo) / range * (h - margin_temp_h * 2 - bottom_axis_h);
        int temp_x = margin_temp_w + i * entry_w;
        graphics_fill_circle(ctx, GPoint(temp_x, h - temp_h - margin_temp_h - bottom_axis_h), 2);
    }

    // Draw a line for the bottom axis
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_line(ctx, GPoint(0, h - bottom_axis_h), GPoint(w, h - bottom_axis_h));
}

void graph_data_layer_create(Layer* parent_layer, GRect frame) {
    s_graph_data_layer = layer_create(frame);
    GRect bounds = layer_get_bounds(s_graph_data_layer);
    layer_set_update_proc(s_graph_data_layer, graph_data_update_proc);
    layer_add_child(parent_layer, s_graph_data_layer);
}

void graph_data_layer_refresh() {

}

void graph_data_layer_destroy() {

}