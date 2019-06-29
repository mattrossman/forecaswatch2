#include "graph_layer.h"
#include "c/appendix/math.h"
#include "c/appendix/persist.h"

static Layer *s_graph_layer;

const int bottom_axis_h = 9;
const int margin_temp_w = 7;
const int margin_temp_h = 7;

static void graph_data_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    int w = bounds.size.w;
    int h = bounds.size.h;

    // Draw a line for the bottom axis
    graphics_context_set_stroke_color(ctx, GColorOrange);
    graphics_draw_line(ctx, GPoint(0, h - bottom_axis_h), GPoint(w, h - bottom_axis_h));
    // And for the left side axis
    graphics_draw_line(ctx, GPoint(0, 0), GPoint(0, h - bottom_axis_h));

    // Allocate a data buffer and load the stored data into it
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
    float entry_w = (float) (bounds.size.w - 2 * margin_temp_w) / (num_entries - 1);
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_context_set_stroke_color(ctx, GColorLightGray);
    // Minimum width a label should cover
    const int label_padding = 17;
    // Round this division up by adding (divisor - 1) to the dividend
    const int entries_per_label = ((float) label_padding + (entry_w - 1)) / entry_w;
    const int font_offset_y = 5; // Adjust for the top whitespace inherent to the font
    for (int i = 0; i < num_entries; ++i) {
        int entry_x = margin_temp_w + i * entry_w;

        // Draw a bar for the precipitation probability
        int precip = precips[i];
        int precip_h = (float) precip / 100.0 * (h - bottom_axis_h);
        graphics_context_set_fill_color(ctx, GColorCyan);
        graphics_fill_rect(ctx, GRect(entry_x - entry_w/2, h - bottom_axis_h - precip_h, entry_w, precip_h), 0, GCornerNone);

        // Draw a point for the temperature reading
        int temp = temps[i];
        int temp_h = (float) (temp - lo) / range * (h - margin_temp_h * 2 - bottom_axis_h);
        graphics_context_set_fill_color(ctx, GColorYellow);
        graphics_fill_circle(ctx, GPoint(entry_x, h - temp_h - margin_temp_h - bottom_axis_h), 1);

        if (i % entries_per_label == 0) {
            // Draw a text hour label at the appropriate interval
            char buf[4];
            snprintf(buf, sizeof(buf), "%d", (forecast_start_hour + i) % 24);
            graphics_draw_text(
                ctx,
                buf,
                fonts_get_system_font(FONT_KEY_GOTHIC_14),
                GRect(entry_x - 20, h - bottom_axis_h - font_offset_y, 40, bottom_axis_h),
                GTextOverflowModeWordWrap,
                GTextAlignmentCenter,
                NULL
            );
        }
        else if ((i + entries_per_label/2) % entries_per_label == 0) {
            // Just draw a tick between hour labels
            graphics_draw_line(ctx,
                GPoint(entry_x, h - bottom_axis_h - 0),
                GPoint(entry_x, h - bottom_axis_h + 4));
        }
    }
}

void graph_layer_create(Layer* parent_layer, GRect frame) {
    s_graph_layer = layer_create(frame);
    GRect bounds = layer_get_bounds(s_graph_layer);
    layer_set_update_proc(s_graph_layer, graph_data_update_proc);
    layer_add_child(parent_layer, s_graph_layer);
}

void graph_layer_refresh() {
    layer_mark_dirty(s_graph_layer);
}

void graph_layer_destroy() {
    layer_destroy(s_graph_layer);
}