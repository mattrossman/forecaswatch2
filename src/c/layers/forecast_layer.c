#include "forecast_layer.h"
#include "c/appendix/persist.h"
#include "c/appendix/math.h"
#include "c/appendix/config.h"
#include "c/appendix/memory_log.h"

#define LEFT_AXIS_LABEL_STRIP_MIN_W 15
#define LEFT_AXIS_LABEL_TO_GRAPH_GAP 2
#define LEFT_AXIS_GRAPH_INSET_DEFAULT (LEFT_AXIS_LABEL_STRIP_MIN_W + LEFT_AXIS_LABEL_TO_GRAPH_GAP)
#define TEMP_LABEL_PAD 2
#define TEMP_LABEL_H 20
#define TEMP_LABEL_MEASURE_BOX_W 200
#define TEMP_LABEL_MEASURE_BOX_H 40
#define BOTTOM_AXIS_FONT_OFFSET 4 // Adjustment for whitespace at top of font
#define LABEL_PADDING 20          // Minimum width a label should cover
#define BOTTOM_AXIS_H 10          // Height of the bottom axis (hour labels)
#define MARGIN_TEMP_H 7           // Height of margins for the temperature plot
#define NIGHT_HATCH_SPACING PBL_IF_COLOR_ELSE(6, 7)
#define NIGHT_HATCH_COLOR GColorDarkGray
#define PRECIP_FILL_COLOR PBL_IF_COLOR_ELSE(GColorCobaltBlue, GColorLightGray)
#define NIGHT_PRECIP_FILL_COLOR PBL_IF_COLOR_ELSE(GColorDukeBlue, GColorLightGray)
#define NIGHT_HATCH_COLOR_PRECIP PBL_IF_COLOR_ELSE(GColorBlue, GColorWhite)
#define NIGHT_BOUNDARY_COLOR PBL_IF_COLOR_ELSE(GColorDarkGray, GColorLightGray)
#define NIGHT_BOUNDARY_COLOR_PRECIP PBL_IF_COLOR_ELSE(GColorVividCerulean, GColorWhite)
#define FORECAST_STEP_SECONDS (60 * 60)
#define DAY_SECONDS (24 * 60 * 60)

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
    time_t timestamp;
    int type; // 0 = sunrise, 1 = sunset
} SunEvent;

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
static int s_axis_left_w = LEFT_AXIS_GRAPH_INSET_DEFAULT;
static int s_label_strip_w = LEFT_AXIS_LABEL_STRIP_MIN_W;
static char s_buffer_lo[12];
static char s_buffer_hi[12];

static RenderSpec make_render_spec()
{
    RenderSpec spec = {
        .draw_night_overlay = g_config->day_night_shading,
        .axis_color = PBL_IF_COLOR_ELSE(GColorOrange, GColorWhite)};

    if (spec.draw_night_overlay)
    {
        spec.axis_color = PBL_IF_COLOR_ELSE(GColorRed, GColorWhite);
    }

    return spec;
}

static ForecastLayout compute_layout(GRect bounds)
{
    ForecastLayout layout;
    layout.graph_bounds = GRect(s_axis_left_w, 0, bounds.size.w - s_axis_left_w, bounds.size.h);
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
    if (!get_valid_sun_events(sun_event_times, &sun_event_start_type))
    {
        return night_segments;
    }

    SunEvent events[6];
    int event_count = 0;

    for (int day_offset = -1; day_offset <= 1; ++day_offset)
    {
        const time_t offset_seconds = (time_t)day_offset * DAY_SECONDS;
        events[event_count++] = (SunEvent){
            .timestamp = sun_event_times[0] + offset_seconds,
            .type = sun_event_start_type};
        events[event_count++] = (SunEvent){
            .timestamp = sun_event_times[1] + offset_seconds,
            .type = 1 - sun_event_start_type};
    }

    for (int i = 1; i < event_count; ++i)
    {
        SunEvent current = events[i];
        int j = i - 1;
        while (j >= 0 && events[j].timestamp > current.timestamp)
        {
            events[j + 1] = events[j];
            --j;
        }
        events[j + 1] = current;
    }

    for (int i = 0; i < event_count - 1; ++i)
    {
        const SunEvent event_start = events[i];
        const SunEvent event_end = events[i + 1];
        if (event_start.type != 1 || event_end.type != 0)
        {
            continue;
        }

        night_segments_add(&night_segments, event_start.timestamp, event_end.timestamp);
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

static int16_t aligned_hatch_start_y(int16_t x, int16_t y_start, int16_t spacing)
{
    int16_t modulo = (x + y_start) % spacing;
    if (modulo < 0)
    {
        modulo += spacing;
    }

    if (modulo == 0)
    {
        return y_start;
    }

    return y_start + (spacing - modulo);
}

static void draw_night_hatch_rect(GContext *ctx, GRect rect, int16_t spacing)
{
    if (spacing <= 0 || rect.size.w <= 0 || rect.size.h <= 0)
    {
        return;
    }

    const int16_t x_end = rect.origin.x + rect.size.w;
    const int16_t y_end = rect.origin.y + rect.size.h;
    for (int16_t x = rect.origin.x; x < x_end; ++x)
    {
        int16_t hatch_y = aligned_hatch_start_y(x, rect.origin.y, spacing);
        for (int16_t y = hatch_y; y < y_end; y += spacing)
        {
            graphics_draw_pixel(ctx, GPoint(x, y));
        }
    }
}

static void draw_night_regions(GContext *ctx, GRect graph_plot_rect, time_t graph_start, time_t graph_end,
                               const NightSegments *night_segments)
{
    if (!night_segments || night_segments->count == 0)
    {
        return;
    }

    const int16_t graph_left = graph_plot_rect.origin.x;
    const int16_t graph_right = graph_plot_rect.origin.x + graph_plot_rect.size.w;

    const int16_t hatch_spacing = NIGHT_HATCH_SPACING;
    const bool is_color = PBL_IF_COLOR_ELSE(true, false);
    graphics_context_set_stroke_color(ctx, is_color ? NIGHT_HATCH_COLOR : GColorWhite);

    for (int i = 0; i < night_segments->count; ++i)
    {
        int16_t x0 = graph_x_for_time(night_segments->segments[i].start, graph_start, graph_end, graph_plot_rect);
        int16_t x1 = graph_x_for_time(night_segments->segments[i].end, graph_start, graph_end, graph_plot_rect);

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

static int16_t clamped_precip_top_y_for_x(GRect graph_plot_rect,
                                          const GPoint *points_precip, int num_entries, int16_t x)
{
    const int16_t y_top_limit = graph_plot_rect.origin.y;
    int16_t precip_y = precip_top_y_for_x(points_precip, num_entries, x);
    if (precip_y < y_top_limit)
    {
        precip_y = y_top_limit;
    }

    return precip_y;
}

static void draw_night_hatch_over_precip(GContext *ctx, GRect graph_plot_rect, time_t graph_start, time_t graph_end,
                                         const NightSegments *night_segments,
                                         const GPoint *points_precip, int num_entries)
{
    if (!night_segments || night_segments->count == 0)
    {
        return;
    }

    const int16_t graph_left = graph_plot_rect.origin.x;
    const int16_t graph_right = graph_plot_rect.origin.x + graph_plot_rect.size.w;
    const int16_t y_bottom_exclusive = graph_plot_rect.origin.y + graph_plot_rect.size.h;
    const int16_t y_bottom_inclusive = y_bottom_exclusive - 1;
    const int16_t hatch_spacing = NIGHT_HATCH_SPACING;
    const bool is_color = PBL_IF_COLOR_ELSE(true, false);

    for (int i = 0; i < night_segments->count; ++i)
    {
        int16_t x0 = graph_x_for_time(night_segments->segments[i].start, graph_start, graph_end, graph_plot_rect);
        int16_t x1 = graph_x_for_time(night_segments->segments[i].end, graph_start, graph_end, graph_plot_rect);

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

        if (is_color)
        {
            graphics_context_set_stroke_color(ctx, NIGHT_PRECIP_FILL_COLOR);
            for (int16_t x = x0; x < x1; ++x)
            {
                const int16_t precip_y = clamped_precip_top_y_for_x(graph_plot_rect, points_precip, num_entries, x);
                if (precip_y <= y_bottom_inclusive)
                {
                    graphics_draw_line(ctx, GPoint(x, precip_y), GPoint(x, y_bottom_inclusive));
                }
            }
        }

        graphics_context_set_stroke_color(ctx, is_color ? NIGHT_HATCH_COLOR_PRECIP : GColorWhite);
        for (int16_t x = x0; x < x1; ++x)
        {
            const int16_t precip_y = clamped_precip_top_y_for_x(graph_plot_rect, points_precip, num_entries, x);
            int16_t hatch_y = aligned_hatch_start_y(x, precip_y, hatch_spacing);
            for (int16_t y = hatch_y; y < y_bottom_exclusive; y += hatch_spacing)
            {
                graphics_draw_pixel(ctx, GPoint(x, y));
            }
        }
    }
}

static void draw_night_boundaries(GContext *ctx, GRect graph_plot_rect, time_t graph_start, time_t graph_end,
                                  const NightSegments *night_segments)
{
    if (!night_segments || night_segments->count == 0)
    {
        return;
    }

    graphics_context_set_stroke_color(ctx, NIGHT_BOUNDARY_COLOR);
    graphics_context_set_stroke_width(ctx, 1);

    const int16_t y0 = graph_plot_rect.origin.y;
    const int16_t y1 = graph_plot_rect.origin.y + graph_plot_rect.size.h - 1;
    for (int i = 0; i < night_segments->count; ++i)
    {
        const time_t segment_start = night_segments->segments[i].start;
        const time_t segment_end = night_segments->segments[i].end;

        if (segment_start > graph_start && segment_start < graph_end)
        {
            const int16_t start_x = graph_x_for_time(segment_start, graph_start, graph_end, graph_plot_rect);
            graphics_draw_line(ctx, GPoint(start_x, y0), GPoint(start_x, y1));
        }

        if (segment_end > graph_start && segment_end < graph_end)
        {
            const int16_t end_x = graph_x_for_time(segment_end, graph_start, graph_end, graph_plot_rect);
            graphics_draw_line(ctx, GPoint(end_x, y0), GPoint(end_x, y1));
        }
    }
}

static void draw_night_boundaries_over_precip(GContext *ctx, GRect graph_plot_rect, time_t graph_start, time_t graph_end,
                                               const NightSegments *night_segments,
                                               const GPoint *points_precip, int num_entries)
{
    if (!night_segments || night_segments->count == 0)
    {
        return;
    }

    graphics_context_set_stroke_color(ctx, NIGHT_BOUNDARY_COLOR_PRECIP);
    graphics_context_set_stroke_width(ctx, 1);

    const int16_t y_bottom = graph_plot_rect.origin.y + graph_plot_rect.size.h - 1;
    for (int i = 0; i < night_segments->count; ++i)
    {
        const time_t segment_start = night_segments->segments[i].start;
        const time_t segment_end = night_segments->segments[i].end;

        if (segment_start > graph_start && segment_start < graph_end)
        {
            const int16_t start_x = graph_x_for_time(segment_start, graph_start, graph_end, graph_plot_rect);
            const int16_t start_precip_y = clamped_precip_top_y_for_x(graph_plot_rect, points_precip, num_entries, start_x);
            graphics_draw_line(ctx, GPoint(start_x, start_precip_y), GPoint(start_x, y_bottom));
        }

        if (segment_end > graph_start && segment_end < graph_end)
        {
            const int16_t end_x = graph_x_for_time(segment_end, graph_start, graph_end, graph_plot_rect);
            const int16_t end_precip_y = clamped_precip_top_y_for_x(graph_plot_rect, points_precip, num_entries, end_x);
            graphics_draw_line(ctx, GPoint(end_x, end_precip_y), GPoint(end_x, y_bottom));
        }
    }
}

static void forecast_update_proc(Layer *layer, GContext *ctx)
{
    MEMORY_LOG_HEAP("forecast_update:enter");
    GRect bounds = layer_get_bounds(layer);
    RenderSpec render_spec = make_render_spec();
    ForecastLayout layout = compute_layout(bounds);
    GRect graph_bounds = layout.graph_bounds;
    GRect graph_plot_rect = layout.graph_plot_rect;
    int w = layout.w;
    int h = layout.h;

    // Load data from storage
    const int num_entries = persist_get_num_entries();
    MemoryHeapProbe redraw_probe = MEMORY_HEAP_PROBE_START("forecast_update");
    if (num_entries < 2)
    {
        graphics_context_set_fill_color(ctx, GColorBlack);
        graphics_fill_rect(ctx, bounds, 0, GCornerNone);
        MEMORY_LOG_HEAP("forecast_update:exit");
        return;
    }

    const time_t forecast_start = persist_get_forecast_start();
    const time_t forecast_end = forecast_start + (num_entries - 1) * FORECAST_STEP_SECONDS;
    NightSegments night_segments = {0};
    struct tm *forecast_start_local = localtime(&forecast_start);
    int16_t temps[num_entries];
    uint8_t precips[num_entries];
    uint8_t winds[num_entries];
    persist_get_temp_trend(temps, num_entries);
    persist_get_precip_trend(precips, num_entries);
    persist_get_wind_trend(winds, num_entries);

    // Allocate point arrays for plots
    GPoint points_temp[num_entries];
    GPoint points_precip[num_entries + 2]; // We need 2 more to complete the area
    GPoint points_wind[num_entries];

    // Calculate the temperature range
    int lo, hi;
    min_max(temps, num_entries, &lo, &hi);
    int range = hi - lo;
    const int temp_plot_h = h - MARGIN_TEMP_H * 2 - BOTTOM_AXIS_H;
    const int range_safe = range > 0 ? range : 1;

    // Draw a bounding box for each data entry (the -1 is since we don't want a gap on either side)
    float entry_w = (float)graph_bounds.size.w / (num_entries - 1);
    if (render_spec.draw_night_overlay)
    {
        night_segments = compute_night_segments(forecast_start, forecast_end);
        draw_night_regions(ctx, graph_plot_rect, forecast_start, forecast_end, &night_segments);
        draw_night_boundaries(ctx, graph_plot_rect, forecast_start, forecast_end, &night_segments);
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
        int temp_h = temp_plot_h / 2;
        if (range > 0)
        {
            temp_h = (int)(((int32_t)(temp - lo) * temp_plot_h) / range_safe);
        }
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
    MEMORY_HEAP_PROBE_SAMPLE("before_precip_path_create", &redraw_probe);
    GPath *path_precip_area_under = gpath_create(&path_info_precip);
    MEMORY_HEAP_PROBE_SAMPLE("after_precip_path_create", &redraw_probe);
    graphics_context_set_fill_color(ctx, PRECIP_FILL_COLOR);
    gpath_draw_filled(ctx, path_precip_area_under);
    MEMORY_HEAP_PROBE_SAMPLE("before_precip_path_destroy", &redraw_probe);
    gpath_destroy(path_precip_area_under);
    MEMORY_HEAP_PROBE_SAMPLE("after_precip_path_destroy", &redraw_probe);

    if (render_spec.draw_night_overlay)
    {
        draw_night_hatch_over_precip(ctx, graph_plot_rect, forecast_start, forecast_end, &night_segments,
                                     points_precip, num_entries);
        draw_night_boundaries_over_precip(ctx, graph_plot_rect, forecast_start, forecast_end, &night_segments,
                                          points_precip, num_entries);
    }

    // Draw the precipitation line
    path_info_precip.num_points = num_entries;
    MEMORY_HEAP_PROBE_SAMPLE("before_precip_top_create", &redraw_probe);
    GPath *path_precip_top = gpath_create(&path_info_precip);
    MEMORY_HEAP_PROBE_SAMPLE("after_precip_top_create", &redraw_probe);
    graphics_context_set_stroke_color(ctx, GColorPictonBlue);
    graphics_context_set_stroke_width(ctx, 1);
    gpath_draw_outline_open(ctx, path_precip_top);
    MEMORY_HEAP_PROBE_SAMPLE("before_precip_top_destroy", &redraw_probe);
    gpath_destroy(path_precip_top);
    MEMORY_HEAP_PROBE_SAMPLE("after_precip_top_destroy", &redraw_probe);

    // Draw the temperature line
    GPathInfo path_info_temp = {
        .num_points = num_entries,
        .points = points_temp};
    MEMORY_HEAP_PROBE_SAMPLE("before_temp_path_create", &redraw_probe);
    GPath *path_temp = gpath_create(&path_info_temp);
    MEMORY_HEAP_PROBE_SAMPLE("after_temp_path_create", &redraw_probe);
    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorRed, GColorWhite));
    graphics_context_set_stroke_width(ctx, 3); // Only odd stroke width values supported
    gpath_draw_outline_open(ctx, path_temp);
    MEMORY_HEAP_PROBE_SAMPLE("before_temp_path_destroy", &redraw_probe);
    gpath_destroy(path_temp);
    MEMORY_HEAP_PROBE_SAMPLE("after_temp_path_destroy", &redraw_probe);

    // Prepare and draw the wind speed line (scaled independently)
    if (g_config && g_config->show_wind_graph) {
    int max_wind = (g_config->wind_max > 0) ? g_config->wind_max : 20;
    for (int i = 0; i < num_entries; ++i) {
        int entry_x = graph_bounds.origin.x + i * entry_w;
        int wind = winds[i];
        if (wind > max_wind) {
            wind = max_wind; // Clamp to configured max so line sticks to top
        }
        int wind_h = (float) wind / max_wind * (h - MARGIN_TEMP_H * 2 - BOTTOM_AXIS_H);
        points_wind[i] = GPoint(entry_x, h - wind_h - MARGIN_TEMP_H - BOTTOM_AXIS_H);
    }
    GPathInfo path_info_wind = {
        .num_points = num_entries,
        .points = points_wind
    };
    GPath *path_wind = gpath_create(&path_info_wind);
    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorYellow, GColorWhite));
    graphics_context_set_stroke_width(ctx, 1);
    gpath_draw_outline_open(ctx, path_wind);
    gpath_destroy(path_wind);
    } // end show_wind_graph

    // Draw a line for the bottom axis
    graphics_context_set_stroke_color(ctx, render_spec.axis_color);
    graphics_context_set_stroke_width(ctx, 1);
    const int16_t axis_y = h - BOTTOM_AXIS_H;
    graphics_draw_line(ctx, GPoint(graph_bounds.origin.x, axis_y), GPoint(graph_bounds.origin.x + w, axis_y));
    // And for the left side axis
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(0, 0, s_axis_left_w, h - BOTTOM_AXIS_H), 0, GCornerNone); // Paint over plot bleeding
    graphics_draw_line(ctx, GPoint(graph_bounds.origin.x, 0), GPoint(graph_bounds.origin.x, axis_y));
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, s_buffer_hi,
                       fonts_get_system_font(FONT_KEY_GOTHIC_18),
                       GRect(0, -3, s_label_strip_w, TEMP_LABEL_H),
                       GTextOverflowModeFill, GTextAlignmentRight, NULL);
    graphics_draw_text(ctx, s_buffer_lo,
                       fonts_get_system_font(FONT_KEY_GOTHIC_18),
                       GRect(0, 22, s_label_strip_w, TEMP_LABEL_H),
                       GTextOverflowModeFill, GTextAlignmentRight, NULL);
    MEMORY_HEAP_PROBE_LOG_MIN(&redraw_probe);
    MEMORY_LOG_HEAP("forecast_update:exit");
}

static int temp_label_string_width(const char *text)
{
    const GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_18);
    const GRect box = GRect(0, 0, TEMP_LABEL_MEASURE_BOX_W, TEMP_LABEL_MEASURE_BOX_H);
    const GSize sz = graphics_text_layout_get_content_size(text, font, box, GTextOverflowModeFill,
                                                           GTextAlignmentRight);
    return sz.w;
}

static void text_labels_refresh()
{
    snprintf(s_buffer_hi, sizeof(s_buffer_hi), "%d", config_localize_temp(persist_get_temp_hi()));
    snprintf(s_buffer_lo, sizeof(s_buffer_lo), "%d", config_localize_temp(persist_get_temp_lo()));

    int content_w = temp_label_string_width(s_buffer_hi);
    const int w_lo = temp_label_string_width(s_buffer_lo);
    if (w_lo > content_w)
    {
        content_w = w_lo;
    }
    content_w += TEMP_LABEL_PAD;

    int label_strip_w = content_w;
    if (label_strip_w < LEFT_AXIS_LABEL_STRIP_MIN_W)
    {
        label_strip_w = LEFT_AXIS_LABEL_STRIP_MIN_W;
    }
    s_label_strip_w = label_strip_w;
    const int graph_inset_w = label_strip_w + LEFT_AXIS_LABEL_TO_GRAPH_GAP;

    if (graph_inset_w != s_axis_left_w)
    {
        s_axis_left_w = graph_inset_w;
    }

}

void forecast_layer_create(Layer *parent_layer, GRect frame)
{
    s_forecast_layer = layer_create(frame);

    // Fill the contents with values

    layer_set_update_proc(s_forecast_layer, forecast_update_proc);
    text_labels_refresh();

    // Add it as a child layer to the Window's root layer
    layer_add_child(parent_layer, s_forecast_layer);
    MEMORY_LOG_HEAP("after_forecast_layer_create");
}

void forecast_layer_refresh()
{
    text_labels_refresh();
    layer_mark_dirty(s_forecast_layer);
#ifdef FCW2_ENABLE_MEMORY_LOGGING
    APP_LOG(APP_LOG_LEVEL_DEBUG, "MEM|forecast_refresh|entries=%d|free=%lu|used=%lu",
            persist_get_num_entries(),
            (unsigned long)heap_bytes_free(),
            (unsigned long)heap_bytes_used());
#endif
}

void forecast_layer_destroy()
{
    MEMORY_LOG_HEAP("forecast_layer_destroy:before");
    layer_destroy(s_forecast_layer);
    MEMORY_LOG_HEAP("forecast_layer_destroy:after");
}
