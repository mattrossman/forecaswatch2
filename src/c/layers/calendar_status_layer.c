#include "calendar_status_layer.h"
#include "battery_layer.h"
#include "c/appendix/config.h"

#define MONTH_FONT_OFFSET 7
#define BATTERY_W 20
#define BATTERY_H 10
#define PADDING 4


static Layer *s_calendar_status_layer;
static TextLayer *s_calendar_month_layer;
static TextLayer *s_calendar_month_layer;
static GBitmap *s_mute_bitmap;
static GBitmap *s_bt_bitmap;
static BitmapLayer *s_mute_bitmap_layer;
static BitmapLayer *s_bt_bitmap_layer;
static GColor *s_bt_palette;

void calendar_status_layer_create(Layer* parent_layer, GRect frame) {
    s_calendar_status_layer = layer_create(frame);
    GRect bounds = layer_get_bounds(s_calendar_status_layer);
    int w = bounds.size.w;
    int h = bounds.size.h;

    // Set up icons
    s_mute_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MUTE);
    s_bt_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_CONNECT);
    
    // Set up color palette
    s_bt_palette = malloc(2*sizeof(GColor));
    s_bt_palette[0] = GColorPictonBlue;
    s_bt_palette[1] = GColorClear;
    gbitmap_set_palette(s_bt_bitmap, s_bt_palette, false);

    s_mute_bitmap_layer = bitmap_layer_create(GRect(PADDING, 0, h, h));
    bitmap_layer_set_compositing_mode(s_mute_bitmap_layer, GCompOpSet);
    bitmap_layer_set_bitmap(s_mute_bitmap_layer, s_mute_bitmap);

    s_bt_bitmap_layer = bitmap_layer_create(GRect(PADDING * 2 + 10, 0, h, h));
    bitmap_layer_set_compositing_mode(s_bt_bitmap_layer, GCompOpSet);
    bitmap_layer_set_bitmap(s_bt_bitmap_layer, s_bt_bitmap);


    // Set up month text layer
    s_calendar_month_layer = text_layer_create(GRect(0, -MONTH_FONT_OFFSET, w, 25));
    text_layer_set_background_color(s_calendar_month_layer, GColorClear);
    text_layer_set_text_alignment(s_calendar_month_layer, GTextAlignmentCenter);
    text_layer_set_text_color(s_calendar_month_layer, GColorWhite);
    text_layer_set_font(s_calendar_month_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));

    calendar_status_layer_refresh();
    layer_add_child(s_calendar_status_layer, bitmap_layer_get_layer(s_mute_bitmap_layer));
    layer_add_child(s_calendar_status_layer, bitmap_layer_get_layer(s_bt_bitmap_layer));
    layer_add_child(s_calendar_status_layer, text_layer_get_layer(s_calendar_month_layer));
    battery_layer_create(s_calendar_status_layer, GRect(w - BATTERY_W - PADDING, 1, BATTERY_W, BATTERY_H));
    layer_add_child(parent_layer, s_calendar_status_layer);
}

void status_icons_refresh() {
    bool show = config_show_qt() && quiet_time_is_active();
    show = true;
    layer_set_hidden(bitmap_layer_get_layer(s_mute_bitmap_layer), !show);
}

void calendar_status_layer_refresh() {
    static char s_buffer_month[10];
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);

    strftime(s_buffer_month, sizeof(s_buffer_month), "%b %Y", tm_now);
    text_layer_set_text(s_calendar_month_layer, s_buffer_month);
    status_icons_refresh();
}

void calendar_status_layer_destroy() {
    free(s_bt_palette);
    gbitmap_destroy(s_mute_bitmap);
    gbitmap_destroy(s_bt_bitmap);
    bitmap_layer_destroy(s_mute_bitmap_layer);
    layer_destroy(s_calendar_status_layer);
}