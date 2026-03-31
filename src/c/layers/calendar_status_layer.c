#include "calendar_status_layer.h"
#include "battery_layer.h"
#include "c/appendix/config.h"
#include "c/appendix/memlog.h"

#define MONTH_FONT_OFFSET 7
#define BATTERY_W 29
#define BATTERY_H 10
#define PADDING 4
#define ICON_SLOT_1 GRect(PADDING, 0, 10, 10)
#define ICON_SLOT_2 GRect(PADDING * 2 + 10, 0, 10, 10)

static Layer *s_calendar_status_layer;
#ifdef PBL_COLOR
static TextLayer *s_calendar_month_layer;
static TextLayer *s_calendar_month_layer;
static GBitmap *s_mute_bitmap;
static GBitmap *s_bt_bitmap;
static GBitmap *s_bt_disconnect_bitmap;
static BitmapLayer *s_mute_bitmap_layer;
static BitmapLayer *s_bt_bitmap_layer;
static BitmapLayer *s_bt_disconnect_bitmap_layer;
static GColor *s_bt_palette;
static GColor *s_bt_disconnect_palette;
static GColor *s_mute_palette;
#else
static bool s_show_qt;
static bool s_show_bt;
static bool s_show_bt_disconnect;
static char s_month_buffer[10];
#endif


static void bitmap_layer_move_frame(BitmapLayer *bitmap_layer, GRect frame) {
    layer_set_frame(bitmap_layer_get_layer(bitmap_layer), frame);
}

#ifndef PBL_COLOR
static void draw_slashed_box(GContext *ctx, GRect rect) {
    graphics_draw_rect(ctx, rect);
    graphics_draw_line(ctx, rect.origin, GPoint(rect.origin.x + rect.size.w - 1, rect.origin.y + rect.size.h - 1));
}

static void calendar_status_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);

    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, s_month_buffer, fonts_get_system_font(FONT_KEY_GOTHIC_18),
        GRect(0, -MONTH_FONT_OFFSET, bounds.size.w, 25),
        GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

    if (s_show_qt) {
        graphics_context_set_stroke_color(ctx, GColorWhite);
        draw_slashed_box(ctx, ICON_SLOT_1);
    }

    if (s_show_bt || s_show_bt_disconnect) {
        GRect slot = s_show_qt ? ICON_SLOT_2 : ICON_SLOT_1;
        graphics_context_set_stroke_color(ctx, GColorWhite);
        if (s_show_bt_disconnect) {
            draw_slashed_box(ctx, slot);
        } else {
            graphics_draw_rect(ctx, slot);
            graphics_draw_line(ctx, GPoint(slot.origin.x + 2, slot.origin.y + 2), GPoint(slot.origin.x + slot.size.w - 3, slot.origin.y + slot.size.h - 3));
        }
    }
}
#endif

void calendar_status_layer_create(Layer* parent_layer, GRect frame) {
    memlog_heap("calendar_status_layer:create:start");
    s_calendar_status_layer = layer_create(frame);
    GRect bounds = layer_get_bounds(s_calendar_status_layer);
    int w = bounds.size.w;

#ifdef PBL_COLOR
    int h = bounds.size.h;

    // Set up icons
    s_mute_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MUTE);
    s_bt_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_CONNECT);
    s_bt_disconnect_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_DISCONNECT);
    
    // Set up color palette
    s_bt_palette = malloc(2*sizeof(GColor));
    s_bt_palette[0] = PBL_IF_COLOR_ELSE(GColorPictonBlue, GColorWhite);
    s_bt_palette[1] = GColorClear;
    gbitmap_set_palette(s_bt_bitmap, s_bt_palette, false);

    s_bt_disconnect_palette = malloc(2*sizeof(GColor));
    s_bt_disconnect_palette[0] = PBL_IF_COLOR_ELSE(GColorRed, GColorWhite);
    s_bt_disconnect_palette[1] = GColorClear;
    gbitmap_set_palette(s_bt_disconnect_bitmap, s_bt_disconnect_palette, false);

    s_mute_palette = malloc(2*sizeof(GColor));
    s_mute_palette[0] = GColorWhite;
    s_mute_palette[1] = GColorClear;
    gbitmap_set_palette(s_mute_bitmap, s_mute_palette, false);

    s_mute_bitmap_layer = bitmap_layer_create(ICON_SLOT_1);
    bitmap_layer_set_compositing_mode(s_mute_bitmap_layer, GCompOpSet);
    bitmap_layer_set_bitmap(s_mute_bitmap_layer, s_mute_bitmap);

    s_bt_bitmap_layer = bitmap_layer_create(ICON_SLOT_2);
    bitmap_layer_set_compositing_mode(s_bt_bitmap_layer, GCompOpSet);
    bitmap_layer_set_bitmap(s_bt_bitmap_layer, s_bt_bitmap);

    s_bt_disconnect_bitmap_layer = bitmap_layer_create(ICON_SLOT_2);
    bitmap_layer_set_compositing_mode(s_bt_disconnect_bitmap_layer, GCompOpSet);
    bitmap_layer_set_bitmap(s_bt_disconnect_bitmap_layer, s_bt_disconnect_bitmap);


    // Set up month text layer
    s_calendar_month_layer = text_layer_create(GRect(0, -MONTH_FONT_OFFSET, w, 25));
    text_layer_set_background_color(s_calendar_month_layer, GColorClear);
    text_layer_set_text_alignment(s_calendar_month_layer, GTextAlignmentCenter);
    text_layer_set_text_color(s_calendar_month_layer, GColorWhite);
    text_layer_set_font(s_calendar_month_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));

    // Set up bluetooth handler
    connection_service_subscribe((ConnectionHandlers) {
        .pebble_app_connection_handler = bluetooth_callback
    });

    calendar_status_layer_refresh();
    layer_add_child(s_calendar_status_layer, bitmap_layer_get_layer(s_mute_bitmap_layer));
    layer_add_child(s_calendar_status_layer, bitmap_layer_get_layer(s_bt_bitmap_layer));
    layer_add_child(s_calendar_status_layer, bitmap_layer_get_layer(s_bt_disconnect_bitmap_layer));
    layer_add_child(s_calendar_status_layer, text_layer_get_layer(s_calendar_month_layer));
#else
    layer_set_update_proc(s_calendar_status_layer, calendar_status_update_proc);
    connection_service_subscribe((ConnectionHandlers) {
        .pebble_app_connection_handler = bluetooth_callback
    });
    calendar_status_layer_refresh();
#endif
    battery_layer_create(s_calendar_status_layer, GRect(w - BATTERY_W - PADDING, 1, BATTERY_W, BATTERY_H));
    layer_add_child(parent_layer, s_calendar_status_layer);
    memlog_heap("calendar_status_layer:create:end");
}

void bluetooth_icons_refresh(bool connected) {
#ifdef PBL_COLOR
    bool show_bt = connected && g_config->show_bt;
    bool show_bt_disconnect = !connected && g_config->show_bt_disconnect;
    layer_set_hidden(bitmap_layer_get_layer(s_bt_bitmap_layer), !show_bt);
    layer_set_hidden(bitmap_layer_get_layer(s_bt_disconnect_bitmap_layer), !show_bt_disconnect);
#else
    s_show_bt = connected && g_config->show_bt;
    s_show_bt_disconnect = !connected && g_config->show_bt_disconnect;
    layer_mark_dirty(s_calendar_status_layer);
#endif
}

void bluetooth_callback(bool connected) {
    bluetooth_icons_refresh(connected);
    if (!connected && g_config->vibe)
        vibes_double_pulse();
}

bool show_qt_icon() {
    return g_config->show_qt && quiet_time_is_active();
}

void status_icons_refresh() {
#ifdef PBL_COLOR
    bool show_qt = show_qt_icon();
    layer_set_hidden(bitmap_layer_get_layer(s_mute_bitmap_layer), !show_qt);
    bitmap_layer_move_frame(s_bt_bitmap_layer, show_qt ? ICON_SLOT_2 : ICON_SLOT_1);
    bitmap_layer_move_frame(s_bt_disconnect_bitmap_layer, show_qt ? ICON_SLOT_2 : ICON_SLOT_1);

    // Ensure bt icons are correct at start
    bluetooth_icons_refresh(connection_service_peek_pebble_app_connection());
#else
    s_show_qt = show_qt_icon();
    bluetooth_icons_refresh(connection_service_peek_pebble_app_connection());
#endif
}

void calendar_status_layer_refresh() {
    memlog_heap("calendar_status_layer:refresh");
#ifdef PBL_COLOR
    static char s_buffer_month[10];
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);

    strftime(s_buffer_month, sizeof(s_buffer_month), "%b %Y", tm_now);
    text_layer_set_text(s_calendar_month_layer, s_buffer_month);
    status_icons_refresh();
#else
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);

    strftime(s_month_buffer, sizeof(s_month_buffer), "%b %Y", tm_now);
    status_icons_refresh();
    layer_mark_dirty(s_calendar_status_layer);
#endif
}

void calendar_status_layer_destroy() {
    battery_layer_destroy();
#ifdef PBL_COLOR
    free(s_bt_palette);
    free(s_bt_disconnect_palette);
    free(s_mute_palette);
    gbitmap_destroy(s_mute_bitmap);
    gbitmap_destroy(s_bt_bitmap);
    gbitmap_destroy(s_bt_disconnect_bitmap);
    bitmap_layer_destroy(s_mute_bitmap_layer);
    bitmap_layer_destroy(s_bt_bitmap_layer);
    bitmap_layer_destroy(s_bt_disconnect_bitmap_layer);
    layer_destroy(s_calendar_status_layer);
#else
    connection_service_unsubscribe();
    if (s_calendar_status_layer) {
        layer_destroy(s_calendar_status_layer);
        s_calendar_status_layer = NULL;
    }
#endif
    memlog_heap("calendar_status_layer:destroy");
}
