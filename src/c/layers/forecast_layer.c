#include "forecast_layer.h"
#include "c/appendix/config.h"
#include "c/appendix/math.h"
#include "c/appendix/persist.h"

#define LEFT_AXIS_MARGIN_W 17
#define BOTTOM_AXIS_FONT_OFFSET 4 // Adjustment for whitespace at top of font
#define LABEL_PADDING 20          // Minimum width a label should cover
#define BOTTOM_AXIS_H 10          // Height of the bottom axis (hour labels)
#define MARGIN_TEMP_H 7           // Height of margins for the temperature plot

static Layer *s_forecast_layer;
static TextLayer *s_hi_layer;
static TextLayer *s_lo_layer;

static void forecast_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GRect graph_bounds = GRect(LEFT_AXIS_MARGIN_W, 0,
                             bounds.size.w - LEFT_AXIS_MARGIN_W, bounds.size.h);
  int w = graph_bounds.size.w;
  int h = graph_bounds.size.h;

  // Load data from storage
  const int num_entries = persist_get_num_entries();
  const time_t forecast_start = persist_get_forecast_start();
  struct tm *forecast_start_local = localtime(&forecast_start);
  int16_t temps[num_entries];
  uint8_t precips[num_entries];
  uint8_t windspeeds[num_entries];
  persist_get_temp_trend(temps, num_entries);
  persist_get_precip_trend(precips, num_entries);
  persist_get_windspeed_trend(windspeeds, num_entries);

  // Allocate point arrays for plots
  GPoint points_temp[num_entries];
  GPoint points_precip[num_entries + 2]; // We need 2 more to complete the area
  GPoint points_windspeed[num_entries];
  // Calculate the temperature range
  int lo, hi;
  min_max(temps, num_entries, &lo, &hi);
  int range = hi - lo;

  // Draw a bounding box for each data entry (the -1 is since we don't want a
  // gap on either side)
  float entry_w = (float)graph_bounds.size.w / (num_entries - 1);
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorLightGray);

  // Round this division up by adding (divisor - 1) to the dividend
  const int entries_per_label =
      ((float)LABEL_PADDING + (entry_w - 1)) / entry_w;
  for (int i = 0; i < num_entries; ++i) {
    int entry_x = graph_bounds.origin.x + i * entry_w;

    // Save a point for the precipitation probability
    int precip = ((precips[i] & 0xF0) >> 4) * 6.6666;
    int precip_h = (float)precip / 100.0 * (h - BOTTOM_AXIS_H);
    points_precip[i] = GPoint(entry_x, h - BOTTOM_AXIS_H - precip_h);
    int windspeed = windspeeds[i];
    if (windspeed > 100)
      windspeed = 100;
    int windspeed_h = (float)windspeed / 100.0 * (h - BOTTOM_AXIS_H);
    points_windspeed[i] = GPoint(entry_x, h - BOTTOM_AXIS_H - windspeed_h);
    // Save a point for the temperature reading
    int temp = temps[i];
    int temp_h =
        (float)(temp - lo) / range * (h - MARGIN_TEMP_H * 2 - BOTTOM_AXIS_H);
    points_temp[i] =
        GPoint(entry_x, h - temp_h - MARGIN_TEMP_H - BOTTOM_AXIS_H);

    if (i % entries_per_label == 0) {
      // Draw a text hour label at the appropriate interval
      char buf[4];
      snprintf(buf, sizeof(buf), "%d",
               config_axis_hour(forecast_start_local->tm_hour + i));
      graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                         GRect(entry_x - 20,
                               h - BOTTOM_AXIS_H - BOTTOM_AXIS_FONT_OFFSET, 40,
                               BOTTOM_AXIS_H),
                         GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    } else if ((i + entries_per_label / 2) % entries_per_label == 0) {
      // Just draw a tick between hour labels
      graphics_draw_line(ctx, GPoint(entry_x, h - BOTTOM_AXIS_H - 0),
                         GPoint(entry_x, h - BOTTOM_AXIS_H + 4));
    }
  }

  // Complete the area under the precipitation
  points_precip[num_entries] =
      GPoint(graph_bounds.origin.x + w, h - BOTTOM_AXIS_H);
  points_precip[num_entries + 1] =
      GPoint(graph_bounds.origin.x, h - BOTTOM_AXIS_H);

  // Fill the precipitation area
  GPathInfo path_info_precip = {.num_points = num_entries + 2,
                                .points = points_precip};
  GPath *path_precip_area_under = gpath_create(&path_info_precip);
  graphics_context_set_fill_color(
      ctx, PBL_IF_COLOR_ELSE(GColorCobaltBlue, GColorLightGray));
  gpath_draw_filled(ctx, path_precip_area_under);
  gpath_destroy(path_precip_area_under);

  // Draw the precipitation line
  path_info_precip.num_points = num_entries;
  GPath *path_precip_top = gpath_create(&path_info_precip);
  graphics_context_set_stroke_color(ctx, GColorPictonBlue);
  graphics_context_set_stroke_width(ctx, 1);
  gpath_draw_outline_open(ctx, path_precip_top);
  gpath_destroy(path_precip_top);

  // Draw the windspeed line
  GPathInfo path_info_windspeed = {.num_points = num_entries,
                                   .points = points_windspeed};
  GPath *path_windspeed = gpath_create(&path_info_windspeed);
  graphics_context_set_stroke_color(
      ctx, PBL_IF_COLOR_ELSE(GColorWhite, GColorWhite));
  graphics_context_set_stroke_width(
      ctx, 3); // Only odd stroke width values supported
  gpath_draw_outline_open(ctx, path_windspeed);
  gpath_destroy(path_windspeed);

  // Draw the temperature line
  GPathInfo path_info_temp = {.num_points = num_entries, .points = points_temp};
  GPath *path_temp = gpath_create(&path_info_temp);
  graphics_context_set_stroke_color(ctx,
                                    PBL_IF_COLOR_ELSE(GColorRed, GColorWhite));
  graphics_context_set_stroke_width(
      ctx, 3); // Only odd stroke width values supported
  gpath_draw_outline_open(ctx, path_temp);
  gpath_destroy(path_temp);

  // Draw a line for the bottom axis
  graphics_context_set_stroke_color(
      ctx, PBL_IF_COLOR_ELSE(GColorOrange, GColorWhite));
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, GPoint(graph_bounds.origin.x, h - BOTTOM_AXIS_H),
                     GPoint(graph_bounds.origin.x + w, h - BOTTOM_AXIS_H));
  // And for the left side axis
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0, 0, LEFT_AXIS_MARGIN_W, h - BOTTOM_AXIS_H), 0,
                     GCornerNone); // Paint over plot bleeding
  graphics_draw_line(ctx, GPoint(graph_bounds.origin.x, 0),
                     GPoint(graph_bounds.origin.x, h - BOTTOM_AXIS_H));
}

static void text_layers_refresh() {
  static char s_buffer_lo[4], s_buffer_hi[4];

  snprintf(s_buffer_hi, sizeof(s_buffer_hi), "%d",
           config_localize_temp(persist_get_temp_hi()));
  text_layer_set_text(s_hi_layer, s_buffer_hi);

  snprintf(s_buffer_lo, sizeof(s_buffer_lo), "%d",
           config_localize_temp(persist_get_temp_lo()));
  text_layer_set_text(s_lo_layer, s_buffer_lo);
}

void forecast_layer_create(Layer *parent_layer, GRect frame) {
  s_forecast_layer = layer_create(frame);

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

  layer_set_update_proc(s_forecast_layer, forecast_update_proc);
  text_layers_refresh();

  // Add it as a child layer to the Window's root layer
  layer_add_child(parent_layer, s_forecast_layer);
}

void forecast_layer_refresh() {
  layer_mark_dirty(s_forecast_layer);
  text_layers_refresh();
}

void forecast_layer_destroy() {
  text_layer_destroy(s_hi_layer);
  text_layer_destroy(s_lo_layer);
  layer_destroy(s_forecast_layer);
}