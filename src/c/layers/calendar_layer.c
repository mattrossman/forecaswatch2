#include "calendar_layer.h"

#define NUM_WEEKS 3
#define DAYS_PER_WEEK 7
#define FONT_OFFSET 3

static Layer *s_calendar_layer;
static TextLayer *s_calendar_text_layers[NUM_WEEKS * DAYS_PER_WEEK];

void calendar_layer_create(Layer* parent_layer, GRect frame) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Creating calendar layer...");
    s_calendar_layer = layer_create(frame);
    GRect bounds = layer_get_bounds(s_calendar_layer);
    int w = bounds.size.w;
    int h = bounds.size.h;
    float box_w = (float) w / DAYS_PER_WEEK;
    float box_h = (float) h / NUM_WEEKS;

    for (int i = 0; i < NUM_WEEKS * DAYS_PER_WEEK; ++i) {
        // Place a text box in that space
        TextLayer *s_box_text_layer = text_layer_create(
            GRect((i % DAYS_PER_WEEK) * box_w, (i / DAYS_PER_WEEK) * box_h - FONT_OFFSET,
                  box_w, box_h + FONT_OFFSET));
        text_layer_set_background_color(s_box_text_layer, GColorClear);
        text_layer_set_text_alignment(s_box_text_layer, GTextAlignmentCenter);
        text_layer_set_text_color(s_box_text_layer, GColorWhite);
        text_layer_set_font(s_box_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
        text_layer_set_text(s_box_text_layer, "16");
        s_calendar_text_layers[i] = s_box_text_layer;
        layer_add_child(s_calendar_layer, text_layer_get_layer(s_box_text_layer));
    }
    layer_add_child(parent_layer, s_calendar_layer);
}

void calendar_layer_refresh() {

}

void calendar_layer_destroy() {
    for (int i = 0; i < NUM_WEEKS * DAYS_PER_WEEK; ++i) {
        text_layer_destroy(s_calendar_text_layers[i]);
    }
    layer_destroy(s_calendar_layer);
}