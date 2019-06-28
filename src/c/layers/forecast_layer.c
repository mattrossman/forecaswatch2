#include "forecast_layer.h"
#include "graph_layer.h"
#include "c/appendix/globals.h"
#include "c/appendix/persist.h"

static Layer *s_forecast_layer;
static Layer *s_graph_layer;
static TextLayer *s_hi_layer;
static TextLayer *s_lo_layer;
char buf_hi[4];
char buf_lo[4];

static void weather_update_proc(Layer *layer, GContext *ctx) {
    // Weather section outline
    GRect bounds = layer_get_bounds(layer);
    graphics_context_set_stroke_color(ctx, GColorBlue);
    graphics_draw_rect(ctx, bounds);
}

static void temp_lo_hi_draw() {
    snprintf(buf_hi, sizeof(buf_hi), "%d", persist_get_temp_hi());
    text_layer_set_text(s_hi_layer, buf_hi);

    snprintf(buf_lo, sizeof(buf_lo), "%d", persist_get_temp_lo());
    text_layer_set_text(s_lo_layer, buf_lo);
}

void forecast_layer_create(Layer *parent_layer, GRect frame) {
    s_forecast_layer = layer_create(frame);
    GRect bounds = layer_get_bounds(s_forecast_layer);

    // Temperature HIGH
    s_hi_layer = text_layer_create(GRect(0, -5, 15, 20));
    text_layer_set_background_color(s_hi_layer, GColorClear);
    text_layer_set_text_alignment(s_hi_layer, GTextAlignmentRight);
    text_layer_set_text_color(s_hi_layer, GColorWhite);
    text_layer_set_font(s_hi_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    layer_add_child(s_forecast_layer, text_layer_get_layer(s_hi_layer));

    // Temperature LOW
    s_lo_layer = text_layer_create(GRect(0, 25, 15, 20));
    text_layer_set_background_color(s_lo_layer, GColorClear);
    text_layer_set_text_alignment(s_lo_layer, GTextAlignmentRight);
    text_layer_set_text_color(s_lo_layer, GColorWhite);
    text_layer_set_font(s_lo_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    layer_add_child(s_forecast_layer, text_layer_get_layer(s_lo_layer));

    // Fill the contents with values
    temp_lo_hi_draw();

    // Set up contents
    graph_layer_create(s_forecast_layer, GRect(17, 0, bounds.size.w-17, bounds.size.h));

    // Add it as a child layer to the Window's root layer
    layer_add_child(parent_layer, s_forecast_layer);
}

void forecast_layer_refresh() {
    temp_lo_hi_draw();
    graph_layer_refresh();
}

void forecast_layer_destroy() {
    text_layer_destroy(s_hi_layer);
    text_layer_destroy(s_lo_layer);
    graph_layer_destroy();
    layer_destroy(s_forecast_layer);
}