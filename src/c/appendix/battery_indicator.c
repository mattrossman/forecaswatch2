#include "battery_indicator.h"
#include "c/services/watch_services.h"

#define BATTERY_NUB_W 2
#define BATTERY_NUB_H 6
#define BATTERY_STROKE 1
#define FILL_PADDING 1
#define ICON_SPACING 3
#define BATTERY_POWER_ICON_W 7

static GBitmap *s_battery_power_bitmap;
/* Must outlive the bitmap: gbitmap_set_palette(..., false) does not take
   ownership of the palette. */
static GColor s_battery_palette[2];

#ifdef PBL_COLOR
GColor battery_indicator_level_color(int level) {
    if (level >= 50)
        return GColorGreen;
    else if (level >= 30)
        return GColorYellow;
    else
        return GColorRed;
}
#endif

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

static void draw_power_icon(GContext *ctx, GRect frame, GBitmap *icon_bitmap) {
    GRect icon_bounds = gbitmap_get_bounds(icon_bitmap);
    int icon_x = frame.origin.x;
    int icon_y = frame.origin.y + (frame.size.h - icon_bounds.size.h) / 2;

    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_bitmap_in_rect(
        ctx,
        icon_bitmap,
        GRect(icon_x, icon_y, icon_bounds.size.w, icon_bounds.size.h));
    graphics_context_set_compositing_mode(ctx, GCompOpAssign);
}

static bool battery_shows_power_icon(BatteryChargeState state) {
    return state.is_charging || state.is_plugged;
}

int battery_indicator_occupied_width(int frame_w) {
    if (battery_shows_power_icon(watch_services_battery_state())) {
        return frame_w;
    }

    return frame_w - (BATTERY_POWER_ICON_W + ICON_SPACING);
}

void battery_indicator_draw(GContext *ctx, GRect frame) {
    const int w = frame.size.w;
    const int h = frame.size.h;
    BatteryChargeState battery_state = watch_services_battery_state();
    int battery_level = battery_state.charge_percent;
    bool show_power_icon = battery_shows_power_icon(battery_state);

    maybe_unload_battery_power_bitmap(show_power_icon);

    int battery_x = frame.origin.x + BATTERY_POWER_ICON_W + ICON_SPACING;
    int battery_total_w = w - (BATTERY_POWER_ICON_W + ICON_SPACING);
    int battery_w = battery_total_w - BATTERY_NUB_W;

    // Fill the battery level
    GRect color_bounds = GRect(
        battery_x + BATTERY_STROKE + FILL_PADDING,
        frame.origin.y + BATTERY_STROKE + FILL_PADDING,
        battery_w - (BATTERY_STROKE + FILL_PADDING) * 2, h - (BATTERY_STROKE + FILL_PADDING) * 2);
    GRect color_area = GRect(
        color_bounds.origin.x, color_bounds.origin.y,
        color_bounds.size.w * (battery_level + 10) / 110, color_bounds.size.h);
#ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, battery_indicator_level_color(battery_level));
#else
    graphics_context_set_fill_color(ctx, GColorWhite);
#endif
    graphics_fill_rect(ctx, color_area, 0, GCornerNone);

    if (show_power_icon) {
        ensure_battery_power_bitmap_loaded();
        draw_power_icon(ctx, frame, s_battery_power_bitmap);
    }

    // Draw the white battery outline
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_stroke_width(ctx, BATTERY_STROKE);
    graphics_draw_rect(ctx, GRect(battery_x, frame.origin.y, battery_w, h));

    // Draw the battery nub on the right
    graphics_draw_rect(
        ctx,
        GRect(battery_x + battery_w - 1,
              frame.origin.y + h / 2 - BATTERY_NUB_H / 2,
              BATTERY_NUB_W + 1, BATTERY_NUB_H));
}

void battery_indicator_unload(void) {
    maybe_unload_battery_power_bitmap(false);
}
