#include "battery_layer.h"
#include "c/appendix/config.h"
#include "c/appendix/memory_log.h"
#include "c/services/watch_services.h"

#define BATTERY_BODY_W 11
#define BATTERY_BODY_H 10
#define BATTERY_NUB_W 2
#define BATTERY_NUB_H 4
#define BATTERY_STROKE 1
#define FILL_PADDING 1
#define TEXT_ICON_SPACING 2
#define BATTERY_TEXT_OFFSET 5


static Layer *s_battery_layer;
static GBitmap *s_battery_power_bitmap;
static GColor s_battery_palette[2];
static bool s_battery_subscribed;
static bool s_battery_palette_initialized;

static void battery_state_handler(BatteryChargeState charge) {
    battery_layer_refresh();
}

#ifdef PBL_COLOR
static GColor get_battery_color(int level) {
    if (level >= 50)
        return GColorGreen;
    else if (level >= 30)
        return GColorYellow;
    else
        return GColorRed;
}
#endif

static void ensure_battery_power_bitmap_loaded(GColor fill_color) {
    GColor icon_color = gcolor_legible_over(fill_color);
    if (!s_battery_power_bitmap) {
        s_battery_power_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_CHARGING);
        s_battery_palette_initialized = false;
    }
    if (!s_battery_palette_initialized || !gcolor_equal(s_battery_palette[0], icon_color)) {
        s_battery_palette[0] = icon_color;
        s_battery_palette[1] = GColorClear;
        gbitmap_set_palette(s_battery_power_bitmap, s_battery_palette, false);
        s_battery_palette_initialized = true;
    }
}

static void maybe_unload_battery_power_bitmap(bool show_power_icon) {
    if (!show_power_icon && s_battery_power_bitmap) {
        gbitmap_destroy(s_battery_power_bitmap);
        s_battery_power_bitmap = NULL;
        s_battery_palette_initialized = false;
    }
}

static void draw_power_icon(GContext *ctx, int body_x, int body_y, GBitmap *icon_bitmap) {
    GRect icon_bounds = gbitmap_get_bounds(icon_bitmap);
    int icon_x = body_x + (BATTERY_BODY_W - icon_bounds.size.w) / 2;
    int icon_y = body_y + (BATTERY_BODY_H - icon_bounds.size.h) / 2;

    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_bitmap_in_rect(
        ctx,
        icon_bitmap,
        GRect(icon_x, icon_y, icon_bounds.size.w, icon_bounds.size.h));
    graphics_context_set_compositing_mode(ctx, GCompOpAssign);
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
    MEMORY_LOG_HEAP("battery_update:enter");
    GRect bounds = layer_get_bounds(layer);
    int w = bounds.size.w;
    int h = bounds.size.h;
    BatteryChargeState battery_state = watch_services_battery_state();
    int battery_level = battery_state.charge_percent;
    bool show_power_icon = battery_state.is_charging || battery_state.is_plugged;
    GColor fill_color = PBL_IF_COLOR_ELSE(get_battery_color(battery_level), config_foreground_color());

    maybe_unload_battery_power_bitmap(show_power_icon);

    const int battery_x = w - BATTERY_BODY_W - BATTERY_NUB_W;
    const int battery_y = (h - BATTERY_BODY_H) / 2;
    const int fill_max_w = BATTERY_BODY_W - (BATTERY_STROKE + FILL_PADDING) * 2;
    int fill_w = fill_max_w * battery_level / 100;
    if (battery_level > 0 && fill_w == 0) {
        fill_w = 1;
    }

#ifdef PBL_PLATFORM_EMERY
    // emery: the larger status row has room for a percentage beside the icon.
    char battery_text[5];
    snprintf(battery_text, sizeof(battery_text), "%d%%", battery_level);
    graphics_context_set_text_color(ctx, config_foreground_color());
    graphics_draw_text(ctx, battery_text,
                       fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(0, battery_y - BATTERY_TEXT_OFFSET,
                             battery_x - TEXT_ICON_SPACING,
                             h - battery_y + BATTERY_TEXT_OFFSET),
                       GTextOverflowModeFill, GTextAlignmentRight, NULL);
#endif

    // Fill the battery level
    GRect color_bounds = GRect(
        battery_x + BATTERY_STROKE + FILL_PADDING, battery_y + BATTERY_STROKE + FILL_PADDING,
        fill_max_w, BATTERY_BODY_H - (BATTERY_STROKE + FILL_PADDING) * 2);
    GRect color_area = GRect(
        color_bounds.origin.x, color_bounds.origin.y,
        fill_w, color_bounds.size.h);
    graphics_context_set_fill_color(ctx, fill_color);
    graphics_fill_rect(ctx, color_area, 0, GCornerNone);

    if (show_power_icon) {
        ensure_battery_power_bitmap_loaded(fill_color);
        draw_power_icon(ctx, battery_x, battery_y, s_battery_power_bitmap);
    }

    // Draw the battery outline
    graphics_context_set_stroke_color(ctx, config_foreground_color());
    graphics_context_set_stroke_width(ctx, BATTERY_STROKE);
    graphics_draw_rect(ctx, GRect(battery_x, battery_y, BATTERY_BODY_W, BATTERY_BODY_H));

    // Draw the battery nub on the right
    graphics_draw_rect(
        ctx, GRect(battery_x + BATTERY_BODY_W - 1,
              battery_y + BATTERY_BODY_H / 2 - BATTERY_NUB_H / 2,
              BATTERY_NUB_W + 1, BATTERY_NUB_H));
    MEMORY_LOG_HEAP("battery_update:exit");
}

void battery_layer_create(Layer* parent_layer, GRect frame) {
    MemoryHeapProbe probe = MEMORY_HEAP_PROBE_START("battery_layer_create");

    s_battery_layer = layer_create(frame);
    MEMORY_HEAP_PROBE_SAMPLE("after_layer_create", &probe);

    layer_set_update_proc(s_battery_layer, battery_update_proc);
    if (!watch_services_battery_is_fixture()) {
        battery_state_service_subscribe(battery_state_handler);
        s_battery_subscribed = true;
    } else {
        s_battery_subscribed = false;
    }
    MEMORY_HEAP_PROBE_SAMPLE("after_battery_subscribe", &probe);
    layer_add_child(parent_layer, s_battery_layer);
    MEMORY_HEAP_PROBE_SAMPLE("after_layer_add_child", &probe);
    MEMORY_LOG_HEAP("after_battery_layer_create");
    MEMORY_HEAP_PROBE_LOG_MIN(&probe);
}

void battery_layer_refresh() {
    if (s_battery_layer) {
        layer_mark_dirty(s_battery_layer);
    }
}

void battery_layer_destroy() {
    MEMORY_LOG_HEAP("battery_layer_destroy:before");
    if (s_battery_subscribed) {
        battery_state_service_unsubscribe();
        s_battery_subscribed = false;
    }
    if (s_battery_power_bitmap) {
        gbitmap_destroy(s_battery_power_bitmap);
        s_battery_power_bitmap = NULL;
        s_battery_palette_initialized = false;
    }
    layer_destroy(s_battery_layer);
    MEMORY_LOG_HEAP("battery_layer_destroy:after");
}
