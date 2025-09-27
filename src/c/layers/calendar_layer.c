#include "calendar_layer.h"
#include "c/appendix/config.h"
#include "c/appendix/persist.h"
#include <time.h>

#define NUM_WEEKS 1
#define DAYS_PER_WEEK 7
#define FONT_OFFSET 5

static Layer *s_calendar_layer;
static TextLayer *s_calendar_text_layers[NUM_WEEKS * DAYS_PER_WEEK];


static struct tm *relative_tm(int days_from_today)
{
    /* Get a time structure for n days from today (only accurate to the day)
    Use this function to avoid edge cases from daylight savings time
    */
    time_t timestamp = time(NULL);
    tm *local_time = localtime(&timestamp);
    // Set arbitrary hour so there's no daylight savings rounding error:
    local_time->tm_hour = 5; 
    timestamp = mktime(local_time) + days_from_today * SECONDS_PER_DAY;
    return localtime(&timestamp);
}

static GColor date_color(struct tm *t, int i) {
    // Get color for a date, considering weekends and holidays
    if (((persist_get_holidays() >> i) & 0x01)==1)
        return g_config->color_us_federal;
    if (t->tm_wday == 0)
        return g_config->color_sunday;
    if (t->tm_wday == 6)
        return g_config->color_saturday;
    return GColorWhite;
}

static void calendar_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    int w = bounds.size.w;
    int h = bounds.size.h;
    float box_w = (float) w / DAYS_PER_WEEK;
    float box_h = (float) h / NUM_WEEKS;
}

void calendar_layer_create(Layer* parent_layer, GRect frame) {
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
        s_calendar_text_layers[i] = s_box_text_layer;
        layer_add_child(s_calendar_layer, text_layer_get_layer(s_box_text_layer));
    }
    layer_set_update_proc(s_calendar_layer, calendar_update_proc);
    calendar_layer_refresh();
    layer_add_child(parent_layer, s_calendar_layer);
}


void calendar_layer_refresh() {
    static char s_calendar_box_buffers[NUM_WEEKS * DAYS_PER_WEEK][4];
    layer_mark_dirty(s_calendar_layer);

    // Fill each box with an appropriate relative day number
    for (int i = 0; i < NUM_WEEKS * DAYS_PER_WEEK; ++i) {
        char *buffer = s_calendar_box_buffers[i];
        struct tm *t = relative_tm(i);
        
        GColor text_color = PBL_IF_COLOR_ELSE(date_color(t,i), GColorWhite);
        text_layer_set_text_color(s_calendar_text_layers[i], text_color);

        bool highlight_holiday = (config_highlight_holidays() && (((persist_get_holidays() >> i) && 0x01)==1));
        bool highlight_sunday = (config_highlight_sundays() && t->tm_wday == 0);
        bool highlight_saturday = (config_highlight_saturdays() && t->tm_wday == 6);
        bool bold = highlight_holiday || highlight_sunday || highlight_saturday;
        text_layer_set_font(s_calendar_text_layers[i],
            fonts_get_system_font(bold ? FONT_KEY_GOTHIC_18_BOLD : FONT_KEY_GOTHIC_18));

        snprintf(buffer, 4, "%d", t->tm_mday);  
        text_layer_set_text(s_calendar_text_layers[i], buffer);
    }
}

void calendar_layer_destroy() {
    for (int i = 0; i < NUM_WEEKS * DAYS_PER_WEEK; ++i) {
        text_layer_destroy(s_calendar_text_layers[i]);
    }
    layer_destroy(s_calendar_layer);
}
