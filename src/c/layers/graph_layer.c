#include "graph_layer.h"
#include "c/appendix/math.h"
#include "c/appendix/persist.h"

#define FONT_14_OFFSET 5  // Adjustment for whitespace at top of font
#define LABEL_PADDING 17  // Minimum width a label should cover
#define BOTTOM_AXIS_H 9  // Height of the bottom axis (hour labels)
#define MARGIN_GRAPH_W 7  // Width of side margins for graph entries
#define MARGIN_TEMP_H 7  // Height of margins for the temperature plot

static Layer *s_graph_layer;

static void graph_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    int w = bounds.size.w;
    int h = bounds.size.h;

    // Draw a line for the bottom axis
    graphics_context_set_stroke_color(ctx, GColorOrange);
    graphics_draw_line(ctx, GPoint(0, h - BOTTOM_AXIS_H), GPoint(w, h - BOTTOM_AXIS_H));
    // And for the left side axis
    graphics_draw_line(ctx, GPoint(0, 0), GPoint(0, h - BOTTOM_AXIS_H));

    // Load data from storage
    const int num_entries = persist_get_num_entries();
    const int forecast_start_hour = persist_get_start_hour();
    int16_t temps[num_entries];
    uint8_t precips[num_entries];
    persist_get_temp_trend(temps, num_entries);
    persist_get_precip_trend(precips, num_entries);

    // Calculate the temperature range
    int lo, hi;
    min_max(temps, num_entries, &lo, &hi);
    int range = hi - lo;

    // Draw a bounding box for each data entry
    float entry_w = (float) (bounds.size.w - 2 * MARGIN_GRAPH_W) / (num_entries - 1);
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_context_set_stroke_color(ctx, GColorLightGray);

    // Round this division up by adding (divisor - 1) to the dividend
    const int entries_per_label = ((float) LABEL_PADDING + (entry_w - 1)) / entry_w;
    for (int i = 0; i < num_entries; ++i) {
        int entry_x = MARGIN_GRAPH_W + i * entry_w;

        // Draw a bar for the precipitation probability
        int precip = precips[i];
        int precip_h = (float) precip / 100.0 * (h - BOTTOM_AXIS_H);
        graphics_context_set_fill_color(ctx, GColorCyan);
        graphics_fill_rect(ctx, GRect(entry_x - entry_w/2, h - BOTTOM_AXIS_H - precip_h, entry_w, precip_h), 0, GCornerNone);

        // Draw a point for the temperature reading
        int temp = temps[i];
        int temp_h = (float) (temp - lo) / range * (h - MARGIN_TEMP_H * 2 - BOTTOM_AXIS_H);
        graphics_context_set_fill_color(ctx, GColorYellow);
        graphics_fill_circle(ctx, GPoint(entry_x, h - temp_h - MARGIN_TEMP_H - BOTTOM_AXIS_H), 1);

        if (i % entries_per_label == 0) {
            // Draw a text hour label at the appropriate interval
            char buf[4];
            snprintf(buf, sizeof(buf), "%d", (forecast_start_hour + i) % 24);
            graphics_draw_text(
                ctx,
                buf,
                fonts_get_system_font(FONT_KEY_GOTHIC_14),
                GRect(entry_x - 20, h - BOTTOM_AXIS_H - FONT_14_OFFSET, 40, BOTTOM_AXIS_H),
                GTextOverflowModeWordWrap,
                GTextAlignmentCenter,
                NULL
            );
        }
        else if ((i + entries_per_label/2) % entries_per_label == 0) {
            // Just draw a tick between hour labels
            graphics_draw_line(ctx,
                GPoint(entry_x, h - BOTTOM_AXIS_H - 0),
                GPoint(entry_x, h - BOTTOM_AXIS_H + 4));
        }
    }
}

void graph_layer_create(Layer* parent_layer, GRect frame) {
    s_graph_layer = layer_create(frame);
    GRect bounds = layer_get_bounds(s_graph_layer);
    layer_set_update_proc(s_graph_layer, graph_update_proc);
    layer_add_child(parent_layer, s_graph_layer);
}

void graph_layer_refresh() {
    layer_mark_dirty(s_graph_layer);
}

void graph_layer_destroy() {
    layer_destroy(s_graph_layer);
}