#include "calendar_status_layer.h"
#include "battery_layer.h"
#include "c/appendix/config.h"
#include "c/appendix/memory_log.h"
#include "c/appendix/status_icons.h"
#include "c/appendix/battery_indicator.h"
#include "c/services/watch_services.h"

#define PADDING 4
#define MONTH_FONT_OFFSET 7
// emery: center icons in the taller status row.
#ifdef PBL_PLATFORM_EMERY
#define STATUS_ICON_Y(bounds_h, icon_h) (((bounds_h) - (icon_h)) / 2)
#define BATTERY_Y(bounds_h) (((bounds_h) - BATTERY_INDICATOR_H) / 2)
#define MONTH_FONT_KEY FONT_KEY_GOTHIC_24
#else
#define STATUS_ICON_Y(bounds_h, icon_h) ((void)(bounds_h), (void)(icon_h), 0)
#define BATTERY_Y(bounds_h) ((void)(bounds_h), 1)
#define MONTH_FONT_KEY FONT_KEY_GOTHIC_18
#endif

static Layer *s_calendar_status_layer;
static char s_calendar_month_text[10];

static GRect month_text_rect(GRect bounds, GFont font) {
#ifdef PBL_PLATFORM_EMERY
    // emery: vertically center month text using measured height to match taller status bar.
    const GRect measure_box = GRect(0, 0, bounds.size.w, bounds.size.h);
    const GSize text_size = graphics_text_layout_get_content_size(
        s_calendar_month_text, font, measure_box, GTextOverflowModeFill, GTextAlignmentCenter);
    const int text_y = ((bounds.size.h - text_size.h) / 2) - 5;
    return GRect(0, text_y, bounds.size.w, text_size.h + 3);
#else
    (void)font;
    return GRect(0, -MONTH_FONT_OFFSET, bounds.size.w, 25);
#endif
}

static void draw_month_text(GContext *ctx, GRect bounds) {
    const GFont month_font = fonts_get_system_font(MONTH_FONT_KEY);
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(
        ctx,
        s_calendar_month_text,
        month_font,
        month_text_rect(bounds, month_font),
        GTextOverflowModeFill,
        GTextAlignmentCenter,
        NULL);
}

static void calendar_status_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);

    status_icons_draw(ctx,
                      GPoint(PADDING, STATUS_ICON_Y(bounds.size.h, STATUS_ICON_SIZE)),
                      PADDING);

    draw_month_text(ctx, bounds);
}

void calendar_status_layer_create(Layer* parent_layer, GRect frame) {
    MemoryHeapProbe probe = MEMORY_HEAP_PROBE_START("calendar_status_layer_create");

    s_calendar_status_layer = layer_create(frame);
    MEMORY_HEAP_PROBE_SAMPLE("after_layer_create", &probe);

    GRect bounds = layer_get_bounds(s_calendar_status_layer);
    int w = bounds.size.w;

    calendar_status_layer_refresh();

    layer_set_update_proc(s_calendar_status_layer, calendar_status_update_proc);
    MEMORY_HEAP_PROBE_SAMPLE("after_update_proc_set", &probe);

    battery_layer_create(s_calendar_status_layer,
                         GRect(w - BATTERY_INDICATOR_W - PADDING, BATTERY_Y(bounds.size.h), BATTERY_INDICATOR_W, BATTERY_INDICATOR_H));
    MEMORY_HEAP_PROBE_SAMPLE("after_battery_layer_create", &probe);

    layer_add_child(parent_layer, s_calendar_status_layer);
    MEMORY_HEAP_PROBE_SAMPLE("after_parent_child_added", &probe);

    MEMORY_LOG_HEAP("after_calendar_status_layer_create");
    MEMORY_HEAP_PROBE_LOG_MIN(&probe);
}

void status_icons_refresh() {
    layer_mark_dirty(s_calendar_status_layer);
}

void calendar_status_layer_refresh() {
    struct tm tm_now = watch_services_localtime();
    strftime(s_calendar_month_text, sizeof(s_calendar_month_text), "%b %Y", &tm_now);
    status_icons_refresh();
}

void calendar_status_layer_destroy() {
    MEMORY_LOG_HEAP("calendar_status_layer_destroy:before");
    battery_layer_destroy();
    status_icons_unload();
    layer_destroy(s_calendar_status_layer);
    MEMORY_LOG_HEAP("calendar_status_layer_destroy:after");
}
