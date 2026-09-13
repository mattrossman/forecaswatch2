#include "stats_layer.h"

#ifdef FCW2_STATS_ENABLED

#include "c/appendix/config.h"
#include "c/appendix/draw.h"
#include "c/appendix/memory_log.h"
#include "c/appendix/stats_metrics.h"
#include "c/appendix/status_icons.h"
#include "c/appendix/battery_indicator.h"
#include "c/services/watch_services.h"
#include <string.h>

#define CELL_PAD_X 3
#define CELL_PAD_Y 0
#define ICON_PAD 3
#define GRID_LINE_COLOR PBL_IF_COLOR_ELSE(GColorDarkGray, GColorWhite)
#define LABEL_COLOR PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite)
#define BAR_TRACK_COLOR PBL_IF_COLOR_ELSE(GColorDarkGray, GColorWhite)
#define BAR_HATCH_SPACING 2
/* Blank rows above the first lit pixel, measured on emulator screenshots: GOTHIC_09
   capitals start 3px below their text box, the status icon bitmaps 1px below
   their top edge, and the battery outline on its first row. */
#define LABEL_INK_TOP 3
#define STATUS_ICON_INK_TOP 1
/* Labels sit this much lower than their stacked slot, eating into the blank
   rows above the value glyphs: more room above the title, a tighter title-value
   gap, and the value itself does not move. */
#define LABEL_DROP 1
/* Measured text widths can come up a pixel short of what the renderer needs. */
#define LABEL_SLACK_W 2
/* Closest a nudged label may come to a grid line. */
#define LABEL_EDGE_GAP 1

// emery: taller 40px tiles take the larger value font and a thicker bar.
#ifdef PBL_PLATFORM_EMERY
#define VALUE_FONT_KEY FONT_KEY_GOTHIC_24_BOLD
#define BAR_H 5
#else
#define VALUE_FONT_KEY FONT_KEY_GOTHIC_18_BOLD
#define BAR_H 3
#endif
#define LABEL_FONT_KEY FONT_KEY_GOTHIC_09

// emery: the heart rate tile draws the last hour as a line behind its value.
#ifdef PBL_PLATFORM_EMERY
#define HR_GRAPH_POINTS 30
/* Resting heart rate is only logged every few minutes, so a fresher read shows
   nothing new; the history read is a firmware call worth rationing. */
#define HR_GRAPH_REFRESH_S (5 * 60)
/* A steady resting pulse varies by a few BPM; without a floor on the range the
   line would magnify that noise into full-height spikes. */
#define HR_GRAPH_MIN_SPAN 20
#define HR_GRAPH_INSET_Y 2
#define HR_GRAPH_COLOR GColorDarkCandyAppleRed
#endif

static Layer *s_stats_layer;

#ifdef PBL_PLATFORM_EMERY
static uint8_t s_hr_points[HR_GRAPH_POINTS];
static int s_hr_valid;
static time_t s_hr_fetched_at;
static bool s_hr_fetched;
#endif

/* Derive both edges from the bounds so the odd pixel of a 45px band lands in the
   bottom row instead of being dropped. */
static GRect stats_cell_rect(GRect bounds, int index) {
    const int col = index % STATS_COLS;
    const int row = index / STATS_COLS;
    const int x0 = (col * bounds.size.w) / STATS_COLS;
    const int x1 = ((col + 1) * bounds.size.w) / STATS_COLS;
    const int y0 = (row * bounds.size.h) / STATS_ROWS;
    const int y1 = ((row + 1) * bounds.size.h) / STATS_ROWS;
    return GRect(x0, y0, x1 - x0, y1 - y0);
}

static GSize measure(const char *text, GFont font, GRect box) {
    return graphics_text_layout_get_content_size(text, font, box, GTextOverflowModeFill,
                                                 GTextAlignmentCenter);
}

static void draw_centered(GContext *ctx, const char *text, GFont font, GRect box) {
    graphics_draw_text(ctx, text, font, box, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}

#ifdef PBL_COLOR
/* Fixed stops so a bar fills red -> green as the value grows, rather than
   recolouring what is already drawn. */
static GColor bar_stop_color(int index) {
    switch (index) {
        case 0:  return GColorRed;
        case 1:  return GColorOrange;
        case 2:  return GColorYellow;
        default: return GColorIslamicGreen;
    }
}
#endif

static void draw_bar(GContext *ctx, GRect bar, int16_t pct, StatBarStyle style) {
    if (bar.size.w <= 0 || bar.size.h <= 0) {
        return;
    }

    const int filled_w = (bar.size.w * pct) / 100;

#ifdef PBL_COLOR
    if (style == STAT_BAR_STYLE_LEVEL) {
        graphics_context_set_fill_color(ctx, battery_indicator_level_color(pct));
        graphics_fill_rect(ctx, GRect(bar.origin.x, bar.origin.y, filled_w, bar.size.h), 0, GCornerNone);
    }
    else {
        /* No gradient API exists, so paint a column at a time. The stop is keyed
           off the full width so a given x keeps its colour as the bar fills. */
        for (int x = 0; x < filled_w; ++x) {
            graphics_context_set_stroke_color(ctx, bar_stop_color((x * 4) / bar.size.w));
            graphics_draw_line(ctx, GPoint(bar.origin.x + x, bar.origin.y),
                               GPoint(bar.origin.x + x, bar.origin.y + bar.size.h - 1));
        }
    }
#else
    (void) style;
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(bar.origin.x, bar.origin.y, filled_w, bar.size.h), 0, GCornerNone);
#endif

    /* Remaining track: dithered on B&W so it reads as "empty" without a fill. */
    const int track_w = bar.size.w - filled_w;
    if (track_w > 0) {
        const GRect track = GRect(bar.origin.x + filled_w, bar.origin.y, track_w, bar.size.h);
#ifdef PBL_COLOR
        graphics_context_set_fill_color(ctx, BAR_TRACK_COLOR);
        graphics_fill_rect(ctx, track, 0, GCornerNone);
#else
        graphics_context_set_stroke_color(ctx, BAR_TRACK_COLOR);
        draw_hatch_rect(ctx, track, BAR_HATCH_SPACING);
#endif
    }
}

/* One layout for every tile, measured once per redraw from the fonts rather than
   from each tile's text. A tile whose metric has no bar (or shows "--") keeps the
   same title and value rows as its neighbours instead of re-centering. */
typedef struct {
    int16_t label_h;
    int16_t value_h;
    bool bar_reserved;
    bool show_label;
} TileLayout;

static GRect cell_inner(GRect cell) {
    return GRect(cell.origin.x + CELL_PAD_X,
                 cell.origin.y + CELL_PAD_Y,
                 cell.size.w - CELL_PAD_X * 2,
                 cell.size.h - CELL_PAD_Y * 2);
}

static TileLayout tile_layout(GRect cell) {
    const GRect inner = cell_inner(cell);
    const GRect measure_box = GRect(0, 0, inner.size.w, cell.size.h * 2);
    TileLayout layout = {
        .label_h = measure("A", fonts_get_system_font(LABEL_FONT_KEY), measure_box).h,
        .value_h = measure("0", fonts_get_system_font(VALUE_FONT_KEY), measure_box).h,
    };
    const int stacked_h = layout.label_h + layout.value_h;

    /* The label is what identifies a tile, so it outranks the bar when space is
       short: short tiles keep label + value and the grid loses its bars, rather
       than ending up as bare numbers. */
    layout.bar_reserved = (stacked_h + BAR_H + 1 <= inner.size.h);
    layout.show_label = (stacked_h <= inner.size.h - (layout.bar_reserved ? BAR_H + 1 : 0));
    return layout;
}

/* Top of the label (or of the value, when labels do not fit) within `cell`. */
static int tile_text_top(GRect cell, const TileLayout *layout) {
    const GRect inner = cell_inner(cell);
    const int text_h = inner.size.h - (layout->bar_reserved ? BAR_H + 1 : 0);
    const int stacked_h = layout->show_label ? layout->label_h + layout->value_h : layout->value_h;
    return inner.origin.y + (text_h - stacked_h) / 2;
}

/* Centres the label on the cell, then nudges it only as far as needed to stay
   within [min_x, max_x], so corner tiles clear the status icons without
   centring in the leftover space. */
static void draw_label(GContext *ctx, const char *label, GRect cell, int y, int label_h,
                       int min_x, int max_x) {
    const GFont font = fonts_get_system_font(LABEL_FONT_KEY);
    const int avail_w = max_x - min_x;
    if (avail_w <= 0) {
        return;
    }

    int label_w = measure(label, font, GRect(0, 0, avail_w, label_h * 2)).w + LABEL_SLACK_W;
    if (label_w > avail_w) {
        label_w = avail_w;
    }

    int x = cell.origin.x + (cell.size.w - label_w) / 2;
    if (x < min_x) {
        x = min_x;
    }
    if (x + label_w > max_x) {
        x = max_x - label_w;
    }

    graphics_context_set_text_color(ctx, LABEL_COLOR);
    graphics_draw_text(ctx, label, font, GRect(x, y, label_w, label_h),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

#ifdef PBL_PLATFORM_EMERY
/* Read lazily from the update proc, so a hidden grid or a layout without the
   heart rate tile never touches the health history. */
static void hr_graph_refresh_cache(void) {
    const time_t now = watch_services_now();
    if (s_hr_fetched && now >= s_hr_fetched_at && now - s_hr_fetched_at < HR_GRAPH_REFRESH_S) {
        return;
    }

    s_hr_fetched = true;
    s_hr_fetched_at = now;
    s_hr_valid = watch_services_health_heart_rate_history(s_hr_points, HR_GRAPH_POINTS);
}

static void draw_hr_graph(GContext *ctx, GRect area) {
    hr_graph_refresh_cache();
    if (s_hr_valid < 2 || area.size.w < 2 || area.size.h < 2) {
        return;
    }

    int lo = 255;
    int hi = 0;
    for (int i = 0; i < HR_GRAPH_POINTS; ++i) {
        if (s_hr_points[i]) {
            lo = s_hr_points[i] < lo ? s_hr_points[i] : lo;
            hi = s_hr_points[i] > hi ? s_hr_points[i] : hi;
        }
    }
    if (hi - lo < HR_GRAPH_MIN_SPAN) {
        lo = (lo + hi - HR_GRAPH_MIN_SPAN) / 2;
        hi = lo + HR_GRAPH_MIN_SPAN;
    }
    const int span = hi - lo;

    graphics_context_set_stroke_color(ctx, HR_GRAPH_COLOR);
    graphics_context_set_stroke_width(ctx, 1);

    /* Gaps between readings are bridged; the line starts and ends at the first
       and last real reading instead of dropping to zero. */
    bool have_prev = false;
    GPoint prev = GPointZero;
    for (int i = 0; i < HR_GRAPH_POINTS; ++i) {
        if (!s_hr_points[i]) {
            continue;
        }

        const GPoint point = GPoint(
            area.origin.x + (i * (area.size.w - 1)) / (HR_GRAPH_POINTS - 1),
            area.origin.y + area.size.h - 1 - ((s_hr_points[i] - lo) * (area.size.h - 1)) / span);
        if (have_prev) {
            graphics_draw_line(ctx, prev, point);
        }
        prev = point;
        have_prev = true;
    }
}
#endif

static void draw_tile(GContext *ctx, GRect cell, StatMetricId id, const TileLayout *layout,
                      int label_min_x, int label_max_x) {
    if (id == STAT_METRIC_NONE) {
        return;
    }

    const GRect inner = cell_inner(cell);
    if (inner.size.w <= 0) {
        return;
    }

    const StatSample sample = stats_metric_sample(id);

    // emery: the heart rate history goes down first so title and value stay on top.
#ifdef PBL_PLATFORM_EMERY
    if (id == STAT_METRIC_HEART_RATE && strcmp(sample.text, "--") != 0) {
        draw_hr_graph(ctx, GRect(inner.origin.x, cell.origin.y + HR_GRAPH_INSET_Y,
                                 inner.size.w, cell.size.h - HR_GRAPH_INSET_Y * 2));
    }
#endif

    int y = tile_text_top(cell, layout);

    if (layout->show_label) {
        draw_label(ctx, stats_metric_label(id), cell, y + LABEL_DROP, layout->label_h,
                   label_min_x, label_max_x);
        y += layout->label_h;
    }

    graphics_context_set_text_color(ctx, GColorWhite);
    draw_centered(ctx, sample.text, fonts_get_system_font(VALUE_FONT_KEY),
                  GRect(inner.origin.x, y, inner.size.w, layout->value_h));

    if (layout->bar_reserved && sample.bar_pct != STAT_BAR_NONE) {
        draw_bar(ctx, GRect(inner.origin.x, inner.origin.y + inner.size.h - BAR_H,
                            inner.size.w, BAR_H),
                 sample.bar_pct, sample.bar_style);
    }
}

static void draw_grid_lines(GContext *ctx, GRect bounds) {
    graphics_context_set_stroke_color(ctx, GRID_LINE_COLOR);
    for (int col = 1; col < STATS_COLS; ++col) {
        const int x = (col * bounds.size.w) / STATS_COLS;
        graphics_draw_line(ctx, GPoint(x, bounds.origin.y),
                           GPoint(x, bounds.origin.y + bounds.size.h - 1));
    }
    for (int row = 1; row < STATS_ROWS; ++row) {
        const int y = (row * bounds.size.h) / STATS_ROWS;
        graphics_draw_line(ctx, GPoint(bounds.origin.x, y),
                           GPoint(bounds.origin.x + bounds.size.w - 1, y));
    }
}

static void stats_update_proc(Layer *layer, GContext *ctx) {
    const GRect bounds = layer_get_bounds(layer);
    const GRect first_cell = stats_cell_rect(bounds, 0);
    const TileLayout layout = tile_layout(first_cell);

    draw_grid_lines(ctx, bounds);

    /* Status furniture sits in the top row's label band, mirroring the calendar
       status row it replaced: quiet-time/bluetooth on the left, battery on the
       right. They align with the labels' undropped position (ignoring
       LABEL_DROP), sitting 1px above the label ink; without labels they fall
       back to a plain inset. */
    const int ink_top = layout.show_label
        ? tile_text_top(first_cell, &layout) + LABEL_INK_TOP
        : bounds.origin.y + ICON_PAD;

    const int icon_w = status_icons_draw(ctx, GPoint(bounds.origin.x + ICON_PAD,
                                                     ink_top - STATUS_ICON_INK_TOP),
                                         ICON_PAD);

    const GRect battery = GRect(bounds.origin.x + bounds.size.w - BATTERY_INDICATOR_W - ICON_PAD,
                                ink_top, BATTERY_INDICATOR_W, BATTERY_INDICATOR_H);
    battery_indicator_draw(ctx, battery);

    /* Only what is actually drawn pushes a label aside: the bolt's slot next to
       the gauge stays free for labels unless the watch is charging. */
    const int icons_right = bounds.origin.x + ICON_PAD + icon_w + ICON_PAD;
    const int battery_left = battery.origin.x + BATTERY_INDICATOR_W
        - battery_indicator_occupied_width(BATTERY_INDICATOR_W) - ICON_PAD;

    for (int i = 0; i < STATS_SLOT_COUNT; ++i) {
        const StatMetricId id = stats_metric_validate_id(g_config->stat_slots[i]);
        const GRect cell = stats_cell_rect(bounds, i);
        /* Labels may use the cell padding when nudged; the clamp in draw_label
           only moves them off-centre as far as they need. */
        int min_x = cell.origin.x + LABEL_EDGE_GAP;
        int max_x = cell.origin.x + cell.size.w - LABEL_EDGE_GAP;
        if (i == 0 && icon_w > 0 && icons_right > min_x) {
            min_x = icons_right;
        }
        if (i == STATS_COLS - 1 && battery_left < max_x) {
            max_x = battery_left;
        }
        draw_tile(ctx, cell, id, &layout, min_x, max_x);
    }
}

bool stats_layer_enabled(void) {
    return true;
}

void stats_layer_create(Layer *parent_layer, GRect frame) {
    MemoryHeapProbe probe = MEMORY_HEAP_PROBE_START("stats_layer_create");

    s_stats_layer = layer_create(frame);
    MEMORY_HEAP_PROBE_SAMPLE("after_layer_create", &probe);

    layer_set_update_proc(s_stats_layer, stats_update_proc);
    layer_add_child(parent_layer, s_stats_layer);
    MEMORY_HEAP_PROBE_SAMPLE("after_parent_child_added", &probe);

    MEMORY_LOG_HEAP("after_stats_layer_create");
    MEMORY_HEAP_PROBE_LOG_MIN(&probe);
}

void stats_layer_set_hidden(bool hidden) {
    layer_set_hidden(s_stats_layer, hidden);
}

void stats_layer_refresh(void) {
    layer_mark_dirty(s_stats_layer);
}

void stats_layer_destroy(void) {
    MEMORY_LOG_HEAP("stats_layer_destroy:before");
    status_icons_unload();
    battery_indicator_unload();
    layer_destroy(s_stats_layer);
    MEMORY_LOG_HEAP("stats_layer_destroy:after");
}

#else /* !FCW2_STATS_ENABLED */

/* aplite has no health API and only 24K of RAM, so the grid compiles away.
   wscript globs every .c for every platform, hence stubs rather than exclusion. */
bool stats_layer_enabled(void) {
    return false;
}

void stats_layer_create(Layer *parent_layer, GRect frame) {
    (void) parent_layer;
    (void) frame;
}

void stats_layer_set_hidden(bool hidden) {
    (void) hidden;
}

void stats_layer_refresh(void) {
}

void stats_layer_destroy(void) {
}

#endif /* FCW2_STATS_ENABLED */
