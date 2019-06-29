#include "forecast_layer.h"
#include "graph_layer.h"
#include "c/appendix/persist.h"
#include "c/appendix/math.h"

#define LEFT_AXIS_MARGIN_W 17
#define BOTTOM_AXIS_FONT_OFFSET 4  // Adjustment for whitespace at top of font
#define LABEL_PADDING 20  // Minimum width a label should cover
#define BOTTOM_AXIS_H 10  // Height of the bottom axis (hour labels)
#define MARGIN_TEMP_H 7  // Height of margins for the temperature plot

static Layer *s_forecast_layer;
static TextLayer *s_hi_layer;
static TextLayer *s_lo_layer;
char buf_hi[4];
char buf_lo[4];

static void forecast_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    GRect graph_bounds = GRect(LEFT_AXIS_MARGIN_W, 0, bounds.size.w - LEFT_AXIS_MARGIN_W, bounds.size.h);
    int w = graph_bounds.size.w;
    int h = graph_bounds.size.h;

    // Load data from storage
    const int num_entries = persist_get_num_entries();
    const int forecast_start_hour = persist_get_start_hour();
    int16_t temps[num_entries];
    uint8_t precips[num_entries];
    persist_get_temp_trend(temps, num_entries);
    persist_get_precip_trend(precips, num_entries);

    // Allocate point arrays for plots
    GPoint points_temp[num_entries];
    GPoint points_precip[num_entries + 2];  // We need 2 more to complete the area

    // Calculate the temperature range
    int lo, hi;
    min_max(temps, num_entries, &lo, &hi);
    int range = hi - lo;

    // Draw a bounding box for each data entry (the -1 is since we don't want a gap on either side)
    float entry_w = (float) graph_bounds.size.w / (num_entries - 1);
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_context_set_stroke_color(ctx, GColorLightGray);

    // Round this division up by adding (divisor - 1) to the dividend
    const int entries_per_label = ((float) LABEL_PADDING + (entry_w - 1)) / entry_w;
    for (int i = 0; i < num_entries; ++i) {
        int entry_x = graph_bounds.origin.x + i * entry_w;

        // Save a point for the precipitation probability
        int precip = precips[i];
        int precip_h = (float) precip / 100.0 * (h - BOTTOM_AXIS_H);
        points_precip[i] = GPoint(entry_x, h - BOTTOM_AXIS_H - precip_h);

        // Save a point for the temperature reading
        int temp = temps[i];
        int temp_h = (float) (temp - lo) / range * (h - MARGIN_TEMP_H * 2 - BOTTOM_AXIS_H);
        points_temp[i] = GPoint(entry_x, h - temp_h - MARGIN_TEMP_H - BOTTOM_AXIS_H);

        if (i % entries_per_label == 0) {
            // Draw a text hour label at the appropriate interval
            char buf[4];
            snprintf(buf, sizeof(buf), "%d", (forecast_start_hour + i) % 24);
            graphics_draw_text(ctx, buf,
                fonts_get_system_font(FONT_KEY_GOTHIC_14),
                GRect(entry_x - 20, h - BOTTOM_AXIS_H - BOTTOM_AXIS_FONT_OFFSET, 40, BOTTOM_AXIS_H),
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

    // Complete the area under the precipitation
    points_precip[num_entries] = GPoint(graph_bounds.origin.x + w, h - BOTTOM_AXIS_H);
    points_precip[num_entries + 1] = GPoint(graph_bounds.origin.x, h - BOTTOM_AXIS_H);

    // Fill the precipitation area
    GPathInfo path_info_precip = {
        .num_points = num_entries + 2,
        .points = points_precip
    };
    GPath *path_precip_area_under = gpath_create(&path_info_precip);
    graphics_context_set_fill_color(ctx, GColorCobaltBlue);
    gpath_draw_filled(ctx, path_precip_area_under);

    // Draw the precipitation line
    path_info_precip.num_points = num_entries;
    GPath *path_precip_top = gpath_create(&path_info_precip);
    graphics_context_set_stroke_color(ctx, GColorPictonBlue);
    graphics_context_set_stroke_width(ctx, 1);
    gpath_draw_outline_open(ctx, path_precip_top);

    // Draw the temperature line
    GPathInfo path_info_temp = {
        .num_points = num_entries,
        .points = points_temp
    };
    GPath *path_temp = gpath_create(&path_info_temp);
    graphics_context_set_stroke_color(ctx, GColorRed);
    graphics_context_set_stroke_width(ctx, 3);  // Only odd stroke width values supported
    gpath_draw_outline_open(ctx, path_temp);

    // Draw a line for the bottom axis
    graphics_context_set_stroke_color(ctx, GColorOrange);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_line(ctx, GPoint(graph_bounds.origin.x, h - BOTTOM_AXIS_H), GPoint(graph_bounds.origin.x + w, h - BOTTOM_AXIS_H));
    // And for the left side axis
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(0, 0, LEFT_AXIS_MARGIN_W, h - BOTTOM_AXIS_H), 0, GCornerNone);  // Paint over plot bleeding
    graphics_draw_line(ctx, GPoint(graph_bounds.origin.x, 0), GPoint(graph_bounds.origin.x, h - BOTTOM_AXIS_H));
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
    s_hi_layer = text_layer_create(GRect(0, -3, 15, 20));
    text_layer_set_background_color(s_hi_layer, GColorClear);
    text_layer_set_text_alignment(s_hi_layer, GTextAlignmentRight);
    text_layer_set_text_color(s_hi_layer, GColorWhite);
    text_layer_set_font(s_hi_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    layer_add_child(s_forecast_layer, text_layer_get_layer(s_hi_layer));

    // Temperature LOW
    s_lo_layer = text_layer_create(GRect(0, 22, 15, 20));
    text_layer_set_background_color(s_lo_layer, GColorClear);
    text_layer_set_text_alignment(s_lo_layer, GTextAlignmentRight);
    text_layer_set_text_color(s_lo_layer, GColorWhite);
    text_layer_set_font(s_lo_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    layer_add_child(s_forecast_layer, text_layer_get_layer(s_lo_layer));

    // Fill the contents with values
    temp_lo_hi_draw();

    // Set up contents
    // graph_layer_create(s_forecast_layer, GRect(LEFT_AXIS_MARGIN_W, 0, bounds.size.w-LEFT_AXIS_MARGIN_W, bounds.size.h));
    layer_set_update_proc(s_forecast_layer, forecast_update_proc);

    // Add it as a child layer to the Window's root layer
    layer_add_child(parent_layer, s_forecast_layer);
}

void forecast_layer_refresh() {
    layer_mark_dirty(s_forecast_layer);
    temp_lo_hi_draw();
    graph_layer_refresh();
}

void forecast_layer_destroy() {
    text_layer_destroy(s_hi_layer);
    text_layer_destroy(s_lo_layer);
    graph_layer_destroy();
    layer_destroy(s_forecast_layer);
}