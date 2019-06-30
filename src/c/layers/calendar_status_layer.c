#include "calendar_status_layer.h"
#include "battery_layer.h"

#define BUFFER_MONTH_SIZE 10
#define MONTH_FONT_OFFSET 7
#define BATTERY_W 15
#define BATTERY_H 8


static Layer *s_calendar_status_layer;
static TextLayer *s_calendar_month_layer;
char buffer_month[BUFFER_MONTH_SIZE];

void calendar_status_layer_create(Layer* parent_layer, GRect frame) {
    s_calendar_status_layer = layer_create(frame);
    GRect bounds = layer_get_bounds(s_calendar_status_layer);
    int w = bounds.size.w;
    int h = bounds.size.h;

    s_calendar_month_layer = text_layer_create(GRect(0, -MONTH_FONT_OFFSET, w, 18));
    text_layer_set_background_color(s_calendar_month_layer, GColorClear);
    text_layer_set_text_alignment(s_calendar_month_layer, GTextAlignmentCenter);
    text_layer_set_text_color(s_calendar_month_layer, GColorWhite);
    text_layer_set_font(s_calendar_month_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));

    calendar_status_layer_refresh();
    layer_add_child(s_calendar_status_layer, text_layer_get_layer(s_calendar_month_layer));
    battery_layer_create(s_calendar_status_layer, GRect(w - BATTERY_W - 4, h / 2 - BATTERY_H / 2, BATTERY_W, BATTERY_H));
    layer_add_child(parent_layer, s_calendar_status_layer);
}

void calendar_status_layer_refresh() {
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);

    strftime(buffer_month, BUFFER_MONTH_SIZE, "%B %Y", tm_now);
    text_layer_set_text(s_calendar_month_layer, buffer_month);
}

void calendar_status_layer_destroy() {
    layer_destroy(s_calendar_status_layer);
}