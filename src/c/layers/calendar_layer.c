#include "calendar_layer.h"
#include <time.h>

#define NUM_WEEKS 3
#define DAYS_PER_WEEK 7
#define FONT_OFFSET 2

static Layer *s_calendar_layer;
static TextLayer *s_calendar_text_layers[NUM_WEEKS * DAYS_PER_WEEK];
static char calendar_box_buffers[NUM_WEEKS * DAYS_PER_WEEK][4];

static void calendar_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    int w = bounds.size.w;
    int h = bounds.size.h;
    float box_w = (float) w / DAYS_PER_WEEK;
    float box_h = (float) h / NUM_WEEKS;

    // Calculate which box holds today's date
    time_t today = time(NULL);
    struct tm *tm_today = localtime(&today);
    const int i_today = 7 + tm_today->tm_wday;

    graphics_context_set_fill_color(ctx, GColorBlue);
    graphics_fill_rect(ctx,
        GRect((i_today % DAYS_PER_WEEK) * box_w, (i_today / DAYS_PER_WEEK) * box_h,
        box_w, box_h), 1, GCornersAll);
}

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
        s_calendar_text_layers[i] = s_box_text_layer;
        layer_add_child(s_calendar_layer, text_layer_get_layer(s_box_text_layer));
    }
    layer_set_update_proc(s_calendar_layer, calendar_update_proc);
    calendar_layer_refresh();
    layer_add_child(parent_layer, s_calendar_layer);
    // APP_LOG(APP_LOG_LEVEL_DEBUG, "The DOM 0 days from today is: %d", relative_day_of_month(0));
}

static int relative_day_of_month(int days_from_today) {
    // What is the day of the month relative to today?
    time_t timestamp = time(NULL);
    timestamp += days_from_today * SECONDS_PER_DAY;
    tm *local_time = localtime(&timestamp);
    return local_time->tm_mday;
}

void calendar_layer_refresh() {
    // Request redraw (of today's highlight)
    layer_mark_dirty(s_calendar_layer);

    // Calculate which box holds today's date
    time_t today = time(NULL);
    struct tm *tm_today = localtime(&today);
    const int i_today = 7 + tm_today->tm_wday;

    // Fill each box with an appropriate relative day number
    for (int i = 0; i < NUM_WEEKS * DAYS_PER_WEEK; ++i) {
        char *buffer = calendar_box_buffers[i];
        snprintf(buffer, 4, "%d", relative_day_of_month(i - i_today));  
        text_layer_set_text(s_calendar_text_layers[i], buffer);
    }
}

void calendar_layer_destroy() {
    for (int i = 0; i < NUM_WEEKS * DAYS_PER_WEEK; ++i) {
        text_layer_destroy(s_calendar_text_layers[i]);
    }
    layer_destroy(s_calendar_layer);
}