#include "battery_layer.h"
#include "c/appendix/persist.h"
#include "c/appendix/memory_log.h"

#define BATTERY_NUB_W 2
#define BATTERY_NUB_H 6
#define BATTERY_STROKE 1
#define FILL_PADDING 1
#define ICON_SPACING 3
#define BATTERY_POWER_ICON_W 7


static Layer *s_battery_layer;
static GBitmap *s_battery_power_bitmap;
static GColor s_battery_palette[2];

static void battery_state_handler(BatteryChargeState charge) {
    battery_layer_refresh();
}

static GColor get_battery_color(int level) {
    if (level >= 50)
        return GColorGreen;
    else if (level >= 30)
        return GColorYellow;
    else
        return GColorRed;
}

static void ensure_battery_power_bitmap_loaded(void) {
    if (!s_battery_power_bitmap) {
        s_battery_power_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_CHARGING);
        s_battery_palette[0] = GColorWhite;
        s_battery_palette[1] = GColorClear;
        gbitmap_set_palette(s_battery_power_bitmap, s_battery_palette, false);
    }
}

static void maybe_unload_battery_power_bitmap(bool show_power_icon) {
    if (!show_power_icon && s_battery_power_bitmap) {
        gbitmap_destroy(s_battery_power_bitmap);
        s_battery_power_bitmap = NULL;
    }
}

static void draw_power_icon(GContext *ctx, int h, GBitmap *icon_bitmap) {
    GRect icon_bounds = gbitmap_get_bounds(icon_bitmap);
    int icon_x = 0;
    int icon_y = (h - icon_bounds.size.h) / 2;

    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_bitmap_in_rect(
        ctx,
        icon_bitmap,
        GRect(icon_x, icon_y, icon_bounds.size.w, icon_bounds.size.h));
    graphics_context_set_compositing_mode(ctx, GCompOpAssign);
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    int w = bounds.size.w;
    int h = bounds.size.h;
    BatteryChargeState battery_state = battery_state_service_peek();
    int battery_level = battery_state.charge_percent;
    bool show_power_icon = battery_state.is_charging || battery_state.is_plugged;

    maybe_unload_battery_power_bitmap(show_power_icon);

    int battery_x = BATTERY_POWER_ICON_W + ICON_SPACING;
    int battery_total_w = w - battery_x;
    int battery_w = battery_total_w - BATTERY_NUB_W;

    // Fill the battery level
    GRect color_bounds = GRect(
        battery_x + BATTERY_STROKE + FILL_PADDING, BATTERY_STROKE + FILL_PADDING,
        battery_w - (BATTERY_STROKE + FILL_PADDING) * 2, h - (BATTERY_STROKE + FILL_PADDING) * 2);
    GRect color_area = GRect(
        color_bounds.origin.x, color_bounds.origin.y,
        color_bounds.size.w * (float) (battery_level + 10) / (100.0 + 10), color_bounds.size.h);
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(get_battery_color(battery_level), GColorWhite));
    graphics_fill_rect(ctx, color_area, 0, GCornerNone);

    if (show_power_icon) {
        ensure_battery_power_bitmap_loaded();
        draw_power_icon(ctx, h, s_battery_power_bitmap);
    }

    // Draw the white battery outline
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_stroke_width(ctx, BATTERY_STROKE);
    graphics_draw_rect(ctx, GRect(battery_x, 0, battery_w, h));

    // Draw the battery nub on the right
    graphics_draw_rect(
        ctx,
        GRect(battery_x + battery_w - 1, h / 2 - BATTERY_NUB_H / 2, BATTERY_NUB_W + 1, BATTERY_NUB_H));
}

void battery_layer_create(Layer* parent_layer, GRect frame) {
    MemoryHeapProbe probe = MEMORY_HEAP_PROBE_START("battery_layer_create");

    s_battery_layer = layer_create(frame);
    MEMORY_HEAP_PROBE_SAMPLE("after_layer_create", &probe);

    layer_set_update_proc(s_battery_layer, battery_update_proc);
    battery_state_service_subscribe(battery_state_handler);
    MEMORY_HEAP_PROBE_SAMPLE("after_battery_subscribe", &probe);
    layer_add_child(parent_layer, s_battery_layer);
    MEMORY_HEAP_PROBE_SAMPLE("after_layer_add_child", &probe);
    MEMORY_LOG_HEAP("after_battery_layer_create");
    MEMORY_HEAP_PROBE_LOG_MIN(&probe);
}

void battery_layer_refresh() {
    layer_mark_dirty(s_battery_layer);
}

void battery_layer_destroy() {
    MEMORY_LOG_HEAP("battery_layer_destroy:before");
    battery_state_service_unsubscribe();
    if (s_battery_power_bitmap) {
        gbitmap_destroy(s_battery_power_bitmap);
        s_battery_power_bitmap = NULL;
    }
    layer_destroy(s_battery_layer);
    MEMORY_LOG_HEAP("battery_layer_destroy:after");
}
