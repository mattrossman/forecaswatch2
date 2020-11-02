#include "calendar_layer.h"
#include "c/appendix/config.h"
#include <time.h>

#define NUM_WEEKS 3
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

static int relative_day_of_month(int days_from_today) {
    // What is the day of the month n days from today?
    tm *local_time = relative_tm(days_from_today);
    return local_time->tm_mday;
}

static bool is_us_federal_holiday(struct tm *t)
{
    // No holidays on weekends (ensures we don't register a false positive for special cases)
    if (t->tm_wday == 0 || t->tm_wday == 6)
        return false;

    // These holidays are on a specific weekday, so no special cases
    if ((t->tm_mon == 0  && t->tm_mday >= 15 && t->tm_mday <= 21 && t->tm_wday == 1) || // MLK Day
        (t->tm_mon == 1  && t->tm_mday >= 15 && t->tm_mday <= 21 && t->tm_wday == 1) || // Washington's Birthday
        (t->tm_mon == 4  && t->tm_mday >= 25 && t->tm_mday <= 31 && t->tm_wday == 1) || // Memorial Day
        (t->tm_mon == 8  && t->tm_mday >= 1  && t->tm_mday <= 7  && t->tm_wday == 1) || // Labor Day
        (t->tm_mon == 9  && t->tm_mday >= 8  && t->tm_mday <= 14 && t->tm_wday == 1) || // Columbus Day
        (t->tm_mon == 10 && t->tm_mday >= 22 && t->tm_mday <= 28 && t->tm_wday == 4))   // Thanksgiving
        return true;

    // These remaining holidays are on a specific day of the month, which get
    // moved if they fall on a weekend
    
    // Friday special cases
    if (t->tm_wday == 5 && (
        (t->tm_mon == 11 && t->tm_mday == 31) || // New Years
        (t->tm_mon == 6  && t->tm_mday == 3)  || // Independence Day
        (t->tm_mon == 10 && t->tm_mday == 10) || // Veterans Day
        (t->tm_mon == 11 && t->tm_mday == 24)))  // Christmas
        return true;
    // Monday special cases
    if (t->tm_wday == 1 && (
        (t->tm_mon == 0  && t->tm_mday == 2)  || // New Years
        (t->tm_mon == 6  && t->tm_mday == 5)  || // Independence Day
        (t->tm_mon == 10 && t->tm_mday == 12) || // Veterans Day
        (t->tm_mon == 11 && t->tm_mday == 26)))  // Christmas
        return true;
    // Non special cases
    if ((t->tm_mon == 0  && t->tm_mday == 1)  || // New Years
        (t->tm_mon == 6  && t->tm_mday == 4)  || // Independence Day
        (t->tm_mon == 10 && t->tm_mday == 11) || // Veterans Day
        (t->tm_mon == 11 && t->tm_mday == 25))   // Christmas
        return true;
    
    // Default to no holiday
    return false;
}

static GColor date_color(struct tm *t) {
    // Get color for a date, considering weekends and holidays
    if (is_us_federal_holiday(t))
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

    // Calculate which box holds today's date
    const int i_today = config_n_today();
    struct tm *t = relative_tm(0);

    GColor background_color = PBL_IF_COLOR_ELSE(date_color(t), GColorWhite);
    graphics_context_set_fill_color(ctx, background_color);
    graphics_fill_rect(ctx,
        GRect((i_today % DAYS_PER_WEEK) * box_w, (i_today / DAYS_PER_WEEK) * box_h,
        box_w, box_h), 1, GCornersAll);
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
    // Request redraw (of today's highlight)
    layer_mark_dirty(s_calendar_layer);

    // Calculate which box holds today's date
    const int i_today = config_n_today();

    // Fill each box with an appropriate relative day number
    for (int i = 0; i < NUM_WEEKS * DAYS_PER_WEEK; ++i) {
        char *buffer = s_calendar_box_buffers[i];
        struct tm *t = relative_tm(i - i_today);
        if (i == i_today) {
            GColor background_color = PBL_IF_COLOR_ELSE(date_color(t), GColorWhite);
            text_layer_set_text_color(s_calendar_text_layers[i],
                gcolor_legible_over(background_color));
            text_layer_set_font(s_calendar_text_layers[i],
                fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
        }
        else {
            GColor text_color = PBL_IF_COLOR_ELSE(date_color(t), GColorWhite);
            text_layer_set_text_color(s_calendar_text_layers[i], text_color);
            text_layer_set_font(s_calendar_text_layers[i],
                fonts_get_system_font(FONT_KEY_GOTHIC_18));
        }
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
