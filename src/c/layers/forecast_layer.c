#include "forecast_layer.h"
#include "c/appendix/persist.h"
#include "c/appendix/math.h"
#include "c/appendix/config.h"

#define LEFT_AXIS_MARGIN_W 17
#define BOTTOM_AXIS_FONT_OFFSET 4 // Adjustment for whitespace at top of font
#define LABEL_PADDING 20          // Minimum width a label should cover
#define BOTTOM_AXIS_H 10          // Height of the bottom axis (hour labels)
#define MARGIN_TEMP_H 7           // Height of margins for the temperature plot
#define NIGHT_HATCH_SPACING PBL_IF_COLOR_ELSE(6, 7)
#define NIGHT_HATCH_COLOR GColorLightGray
#define NIGHT_HATCH_COLOR_PRECIP PBL_IF_COLOR_ELSE(GColorPictonBlue, GColorWhite)
#define NIGHT_BOUNDARY_COLOR PBL_IF_COLOR_ELSE(GColorLightGray, GColorLightGray)
#define NIGHT_BOUNDARY_COLOR_PRECIP PBL_IF_COLOR_ELSE(GColorPictonBlue, GColorWhite)
#define FORECAST_STEP_SECONDS (60 * 60)

typedef struct
{
    time_t start;
    time_t end;
} NightSegment;

typedef struct
{
    int count;
    NightSegment segments[3];
} NightSegments;

typedef struct
{
    bool draw_night_overlay;
    GColor axis_color;
} RenderSpec;

typedef struct
{
    GRect graph_bounds;
    GRect graph_plot_rect;
    int16_t w;
    int16_t h;
} ForecastLayout;

static Layer *s_forecast_layer;
static TextLayer *s_hi_layer;
static TextLayer *s_lo_layer;

static RenderSpec make_render_spec()
{
    RenderSpec spec = {
        .draw_night_overlay = g_config->night_shading,
        .axis_color = PBL_IF_COLOR_ELSE(GColorOrange, GColorWhite)};

    if (spec.draw_night_overlay)
    {
        spec.axis_color = PBL_IF_COLOR_ELSE(GColorRed, GColorWhite);
    }

    return spec;
}

static ForecastLayout compute_layout(GRect bounds, RenderSpec spec)
{
    (void)spec;

    ForecastLayout layout = {
        .graph_bounds = GRect(LEFT_AXIS_MARGIN_W, 0, bounds.size.w - LEFT_AXIS_MARGIN_W, bounds.size.h)};
    layout.graph_plot_rect = GRect(layout.graph_bounds.origin.x, 0, layout.graph_bounds.size.w, layout.graph_bounds.size.h - BOTTOM_AXIS_H);
    layout.w = layout.graph_bounds.size.w;
    layout.h = layout.graph_bounds.size.h;

    return layout;
}

static void night_segments_add(NightSegments *night_segments, time_t start, time_t end)
{
    if (night_segments->count >= (int)(sizeof(night_segments->segments) / sizeof(night_segments->segments[0])) || end <= start)
    {
        return;
    }

    night_segments->segments[night_segments->count].start = start;
    night_segments->segments[night_segments->count].end = end;
    night_segments->count += 1;
}

static bool get_valid_sun_events(time_t sun_event_times[2], int *sun_event_start_type)
{
    const int num_sun_events = 2;
    const int sun_events_read = persist_get_sun_event_times(sun_event_times, num_sun_events);
    if (sun_events_read < (int)(sizeof(time_t) * num_sun_events))
    {
        return false;
    }

    const int start_type = persist_get_sun_event_start_type();
    if ((start_type != 0 && start_type != 1) || sun_event_times[0] <= 0 || sun_event_times[1] <= 0 || sun_event_times[1] <= sun_event_times[0])
    {
        return false;
    }

    if (sun_event_start_type)
    {
        *sun_event_start_type = start_type;
    }

    return true;
}

static NightSegments compute_night_segments(time_t graph_start, time_t graph_end)
{
    NightSegments night_segments = {0};

    if (graph_end <= graph_start)
    {
        return night_segments;
    }

    time_t sun_event_times[2] = {0, 0};
    int sun_event_start_type;
    const int num_sun_events = sizeof(sun_event_times) / sizeof(sun_event_times[0]);
    if (!get_valid_sun_events(sun_event_times, &sun_event_start_type))
    {
        return night_segments;
    }

    // Start type is the type of the first event in the list:
    // 0 = sunrise, 1 = sunset. Before sunrise is night; before sunset is day.
    bool is_night = (sun_event_start_type == 0);

    for (int i = 0; i < num_sun_events; ++i)
    {
        if (sun_event_times[i] <= graph_start)
        {
            is_night = !is_night;
        }
    }

    time_t segment_start = graph_start;
    for (int i = 0; i < num_sun_events; ++i)
    {
        const time_t transition = sun_event_times[i];

        if (transition <= graph_start)
        {
            continue;
        }
        if (transition >= graph_end)
        {
            break;
        }

        if (is_night)
        {
            night_segments_add(&night_segments, segment_start, transition);
        }

        segment_start = transition;
        is_night = !is_night;
    }

    if (is_night)
    {
        night_segments_add(&night_segments, segment_start, graph_end);
    }

    return night_segments;
}

static int16_t graph_x_for_time(time_t timestamp, time_t graph_start, time_t graph_end, GRect graph_plot_rect)
{
    const int16_t graph_left = graph_plot_rect.origin.x;
    const int16_t graph_right = graph_plot_rect.origin.x + graph_plot_rect.size.w;

    if (timestamp <= graph_start)
    {
        return graph_left;
    }
    if (timestamp >= graph_end)
    {
        return graph_right;
    }

    const int64_t elapsed = (int64_t)timestamp - graph_start;
    const int64_t total = (int64_t)graph_end - graph_start;
    return graph_left + (int16_t)((elapsed * graph_plot_rect.size.w) / total);
}

static void draw_night_hatch_rect(GContext *ctx, GRect rect, int16_t spacing)
{
    if (spacing <= 0 || rect.size.w <= 0 || rect.size.h <= 0)
    {
        return;
    }

    const int16_t x_end = rect.origin.x + rect.size.w;
    const int16_t y_end = rect.origin.y + rect.size.h;
    for (int16_t y = rect.origin.y; y < y_end; ++y)
    {
        for (int16_t x = rect.origin.x; x < x_end; ++x)
        {
            if (((x + y) % spacing) == 0)
            {
                graphics_draw_pixel(ctx, GPoint(x, y));
            }
        }
    }
}

static void draw_night_regions(GContext *ctx, GRect graph_plot_rect, time_t graph_start, time_t graph_end)
{
    const NightSegments night_segments = compute_night_segments(graph_start, graph_end);
    if (night_segments.count == 0)
    {
        return;
    }

    const int16_t graph_left = graph_plot_rect.origin.x;
    const int16_t graph_right = graph_plot_rect.origin.x + graph_plot_rect.size.w;

    const int16_t hatch_spacing = NIGHT_HATCH_SPACING;
    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(NIGHT_HATCH_COLOR, GColorWhite));

    for (int i = 0; i < night_segments.count; ++i)
    {
        int16_t x0 = graph_x_for_time(night_segments.segments[i].start, graph_start, graph_end, graph_plot_rect);
        int16_t x1 = graph_x_for_time(night_segments.segments[i].end, graph_start, graph_end, graph_plot_rect);

        if (x0 < graph_left)
        {
            x0 = graph_left;
        }
        if (x1 > graph_right)
        {
            x1 = graph_right;
        }
        if (x1 <= x0)
        {
            continue;
        }

        GRect night_rect = GRect(x0, graph_plot_rect.origin.y, x1 - x0, graph_plot_rect.size.h);
        draw_night_hatch_rect(ctx, night_rect, hatch_spacing);
    }
}

static int16_t precip_top_y_for_x(const GPoint *points_precip, int num_entries, int16_t x)
{
    if (x <= points_precip[0].x)
    {
        return points_precip[0].y;
    }

    for (int i = 0; i < num_entries - 1; ++i)
    {
        const int16_t x0 = points_precip[i].x;
        const int16_t y0 = points_precip[i].y;
        const int16_t x1 = points_precip[i + 1].x;
        const int16_t y1 = points_precip[i + 1].y;

        if (x > x1)
        {
            continue;
        }

        if (x1 == x0)
        {
            return y0 < y1 ? y0 : y1;
        }

        return y0 + (int16_t)(((int32_t)(y1 - y0) * (x - x0)) / (x1 - x0));
    }

    return points_precip[num_entries - 1].y;
}

static void draw_night_hatch_over_precip(GContext *ctx, GRect graph_plot_rect, time_t graph_start, time_t graph_end,
                                         const GPoint *points_precip, int num_entries)
{
    const NightSegments night_segments = compute_night_segments(graph_start, graph_end);
    if (night_segments.count == 0)
    {
        return;
    }

    const int16_t graph_left = graph_plot_rect.origin.x;
    const int16_t graph_right = graph_plot_rect.origin.x + graph_plot_rect.size.w;
    const int16_t y_top_limit = graph_plot_rect.origin.y;
    const int16_t y_bottom = graph_plot_rect.origin.y + graph_plot_rect.size.h;
    const int16_t hatch_spacing = NIGHT_HATCH_SPACING;

    graphics_context_set_stroke_color(ctx, NIGHT_HATCH_COLOR_PRECIP);

    for (int i = 0; i < night_segments.count; ++i)
    {
        int16_t x0 = graph_x_for_time(night_segments.segments[i].start, graph_start, graph_end, graph_plot_rect);
        int16_t x1 = graph_x_for_time(night_segments.segments[i].end, graph_start, graph_end, graph_plot_rect);

        if (x0 < graph_left)
        {
            x0 = graph_left;
        }
        if (x1 > graph_right)
        {
            x1 = graph_right;
        }
        if (x1 <= x0)
        {
            continue;
        }

        for (int16_t x = x0; x < x1; ++x)
        {
            int16_t precip_y = precip_top_y_for_x(points_precip, num_entries, x);
            if (precip_y < y_top_limit)
            {
                precip_y = y_top_limit;
            }

            for (int16_t y = precip_y; y < y_bottom; ++y)
            {
                if (((x + y) % hatch_spacing) == 0)
                {
                    graphics_draw_pixel(ctx, GPoint(x, y));
                }
            }
        }
    }
}

static void draw_night_boundaries(GContext *ctx, GRect graph_plot_rect, time_t graph_start, time_t graph_end)
{
    time_t sun_event_times[2] = {0, 0};
    if (!get_valid_sun_events(sun_event_times, NULL))
    {
        return;
    }

    graphics_context_set_stroke_color(ctx, NIGHT_BOUNDARY_COLOR);
    graphics_context_set_stroke_width(ctx, 1);

    const int16_t y0 = graph_plot_rect.origin.y;
    const int16_t y1 = graph_plot_rect.origin.y + graph_plot_rect.size.h - 1;
    for (int i = 0; i < (int)(sizeof(sun_event_times) / sizeof(sun_event_times[0])); ++i)
    {
        const time_t transition = sun_event_times[i];
        if (transition <= graph_start || transition >= graph_end)
        {
            continue;
        }

        const int16_t x = graph_x_for_time(transition, graph_start, graph_end, graph_plot_rect);
        graphics_draw_line(ctx, GPoint(x, y0), GPoint(x, y1));
    }
}

static void draw_night_boundaries_over_precip(GContext *ctx, GRect graph_plot_rect, time_t graph_start, time_t graph_end,
                                              const GPoint *points_precip, int num_entries)
{
    time_t sun_event_times[2] = {0, 0};
    if (!get_valid_sun_events(sun_event_times, NULL))
    {
        return;
    }

    graphics_context_set_stroke_color(ctx, NIGHT_BOUNDARY_COLOR_PRECIP);
    graphics_context_set_stroke_width(ctx, 1);

    const int16_t y_top_limit = graph_plot_rect.origin.y;
    const int16_t y_bottom = graph_plot_rect.origin.y + graph_plot_rect.size.h - 1;
    for (int i = 0; i < (int)(sizeof(sun_event_times) / sizeof(sun_event_times[0])); ++i)
    {
        const time_t transition = sun_event_times[i];
        if (transition <= graph_start || transition >= graph_end)
        {
            continue;
        }

        const int16_t x = graph_x_for_time(transition, graph_start, graph_end, graph_plot_rect);
        int16_t precip_y = precip_top_y_for_x(points_precip, num_entries, x);
        if (precip_y < y_top_limit)
        {
            precip_y = y_top_limit;
        }
        graphics_draw_line(ctx, GPoint(x, precip_y), GPoint(x, y_bottom));
    }
}

static void forecast_update_proc(Layer *layer, GContext *ctx)
{
    GRect bounds = layer_get_bounds(layer);
    RenderSpec render_spec = make_render_spec();
    ForecastLayout layout = compute_layout(bounds, render_spec);
    GRect graph_bounds = layout.graph_bounds;
    GRect graph_plot_rect = layout.graph_plot_rect;
    int w = layout.w;
    int h = layout.h;

    // Load data from storage
    const int num_entries = persist_get_num_entries();
    if (num_entries < 2)
    {
        return;
    }

    const time_t forecast_start = persist_get_forecast_start();
    const time_t forecast_end = forecast_start + (num_entries - 1) * FORECAST_STEP_SECONDS;
    struct tm *forecast_start_local = localtime(&forecast_start);
    int16_t temps[num_entries];
    uint8_t precips[num_entries];
    persist_get_temp_trend(temps, num_entries);
    persist_get_precip_trend(precips, num_entries);

    // Allocate point arrays for plots
    GPoint points_temp[num_entries];
    GPoint points_precip[num_entries + 2]; // We need 2 more to complete the area

    // Calculate the temperature range
    int lo, hi;
    min_max(temps, num_entries, &lo, &hi);
    int range = hi - lo;

    // Draw a bounding box for each data entry (the -1 is since we don't want a gap on either side)
    float entry_w = (float)graph_bounds.size.w / (num_entries - 1);
    if (render_spec.draw_night_overlay)
    {
        draw_night_regions(ctx, graph_plot_rect, forecast_start, forecast_end);
        draw_night_boundaries(ctx, graph_plot_rect, forecast_start, forecast_end);
    }

    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_context_set_stroke_color(ctx, GColorLightGray);

    // Round this division up by adding (divisor - 1) to the dividend
    const int entries_per_label = ((float)LABEL_PADDING + (entry_w - 1)) / entry_w;
    for (int i = 0; i < num_entries; ++i)
    {
        int entry_x = graph_bounds.origin.x + i * entry_w;

        // Save a point for the precipitation probability
        int precip = precips[i];
        int precip_h = (float)precip / 100.0 * (h - BOTTOM_AXIS_H);
        points_precip[i] = GPoint(entry_x, h - BOTTOM_AXIS_H - precip_h);

        // Save a point for the temperature reading
        int temp = temps[i];
        int temp_h = (float)(temp - lo) / range * (h - MARGIN_TEMP_H * 2 - BOTTOM_AXIS_H);
        points_temp[i] = GPoint(entry_x, h - temp_h - MARGIN_TEMP_H - BOTTOM_AXIS_H);

        if (i % entries_per_label == 0)
        {
            // Draw a text hour label at the appropriate interval
            char buf[4];
            snprintf(buf, sizeof(buf), "%d", config_axis_hour(forecast_start_local->tm_hour + i));
            graphics_draw_text(ctx, buf,
                               fonts_get_system_font(FONT_KEY_GOTHIC_14),
                               GRect(entry_x - 20, h - BOTTOM_AXIS_H - BOTTOM_AXIS_FONT_OFFSET, 40, BOTTOM_AXIS_H),
                               GTextOverflowModeWordWrap,
                               GTextAlignmentCenter,
                               NULL);
        }
        else if ((i + entries_per_label / 2) % entries_per_label == 0)
        {
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
        .points = points_precip};
    GPath *path_precip_area_under = gpath_create(&path_info_precip);
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorCobaltBlue, GColorLightGray));
    gpath_draw_filled(ctx, path_precip_area_under);
    gpath_destroy(path_precip_area_under);

    if (render_spec.draw_night_overlay)
    {
        draw_night_hatch_over_precip(ctx, graph_plot_rect, forecast_start, forecast_end, points_precip, num_entries);
        draw_night_boundaries_over_precip(ctx, graph_plot_rect, forecast_start, forecast_end, points_precip, num_entries);
    }

    // Draw the precipitation line
    path_info_precip.num_points = num_entries;
    GPath *path_precip_top = gpath_create(&path_info_precip);
    graphics_context_set_stroke_color(ctx, GColorPictonBlue);
    graphics_context_set_stroke_width(ctx, 1);
    gpath_draw_outline_open(ctx, path_precip_top);
    gpath_destroy(path_precip_top);

    // Draw the temperature line
    GPathInfo path_info_temp = {
        .num_points = num_entries,
        .points = points_temp};
    GPath *path_temp = gpath_create(&path_info_temp);
    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorRed, GColorWhite));
    graphics_context_set_stroke_width(ctx, 3); // Only odd stroke width values supported
    gpath_draw_outline_open(ctx, path_temp);
    gpath_destroy(path_temp);

    // Draw a line for the bottom axis
    graphics_context_set_stroke_color(ctx, render_spec.axis_color);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_line(ctx, GPoint(graph_bounds.origin.x, h - BOTTOM_AXIS_H), GPoint(graph_bounds.origin.x + w, h - BOTTOM_AXIS_H));
    // And for the left side axis
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(0, 0, LEFT_AXIS_MARGIN_W, h - BOTTOM_AXIS_H), 0, GCornerNone); // Paint over plot bleeding
    graphics_draw_line(ctx, GPoint(graph_bounds.origin.x, 0), GPoint(graph_bounds.origin.x, h - BOTTOM_AXIS_H));
}

static void text_layers_refresh()
{
    static char s_buffer_lo[4], s_buffer_hi[4];

    snprintf(s_buffer_hi, sizeof(s_buffer_hi), "%d", config_localize_temp(persist_get_temp_hi()));
    text_layer_set_text(s_hi_layer, s_buffer_hi);

    snprintf(s_buffer_lo, sizeof(s_buffer_lo), "%d", config_localize_temp(persist_get_temp_lo()));
    text_layer_set_text(s_lo_layer, s_buffer_lo);
}

void forecast_layer_create(Layer *parent_layer, GRect frame)
{
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

void forecast_layer_refresh()
{
    layer_mark_dirty(s_forecast_layer);
    text_layers_refresh();
}

void forecast_layer_destroy()
{
    text_layer_destroy(s_hi_layer);
    text_layer_destroy(s_lo_layer);
    layer_destroy(s_forecast_layer);
}
