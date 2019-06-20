#include "graph_data_layer.h"
#include "c/constants.h"

static Layer *s_graph_data_layer;

static void graph_data_update_proc(Layer *layer, GContext *ctx) {
    // Weather section outline
    GRect bounds = layer_get_bounds(layer);
    graphics_context_set_stroke_color(ctx, GColorRed);
    graphics_draw_rect(ctx, bounds);

    // Draw a bounding box for each data entry
    float entry_w = (float) bounds.size.w / c_num_graph_hours;
    graphics_context_set_stroke_color(ctx, GColorYellow);
    for (int i = 0; i < c_num_graph_hours; ++i) {
        graphics_draw_rect(ctx, GRect(i * entry_w, 0, entry_w, bounds.size.h));
    }
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