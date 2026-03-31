#include "calendar_layer.h"
#include "c/appendix/config.h"
#include "c/appendix/memlog.h"
#include <time.h>

#define NUM_WEEKS 3
#define DAYS_PER_WEEK 7
#define FONT_OFFSET 5

static Layer *s_calendar_layer;

/* Copy struct tm out of localtime's static buffer — see localtime(3). */
static struct tm relative_tm(int days_from_today)
{
    /* Get a time structure for n days from today (only accurate to the day)
    Use this function to avoid edge cases from daylight savings time
    */
    time_t timestamp = time(NULL);
    struct tm *local_time = localtime(&timestamp);
    // Set arbitrary hour so there's no daylight savings rounding error:
    local_time->tm_hour = 5;
    timestamp = mktime(local_time) + days_from_today * SECONDS_PER_DAY;
    struct tm *result = localtime(&timestamp);
    struct tm out = *result;
    return out;
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

#ifdef PBL_COLOR
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
#endif

static GColor today_color() {
    // Either follow the date color or override to configured value
#ifdef PBL_COLOR
    struct tm t = relative_tm(0);
    return gcolor_equal(g_config->color_today, GColorBlack) ? date_color(&t) : g_config->color_today;
#else
    return GColorWhite;
#endif
}

static void draw_day_cell(GContext *ctx, int i, int i_today, GRect bounds, float box_w, float box_h)
{
    struct tm t = relative_tm(i - i_today);
    bool highlight_holiday = (config_highlight_holidays() && is_us_federal_holiday(&t));
    bool highlight_sunday = (config_highlight_sundays() && t.tm_wday == 0);
    bool highlight_saturday = (config_highlight_saturdays() && t.tm_wday == 6);
    bool bold = (i == i_today) || highlight_holiday || highlight_sunday || highlight_saturday;

    GRect cell = GRect(
        (i % DAYS_PER_WEEK) * box_w,
        (i / DAYS_PER_WEEK) * box_h,
        box_w,
        box_h);

    if (i == i_today) {
        graphics_context_set_fill_color(ctx, today_color());
        graphics_fill_rect(ctx, cell, 1, GCornersAll);
    }

    static char s_buffer[4];
    snprintf(s_buffer, sizeof(s_buffer), "%d", t.tm_mday);

    graphics_context_set_text_color(ctx,
        i == i_today ? gcolor_legible_over(today_color()) : PBL_IF_COLOR_ELSE(date_color(&t), GColorWhite));
    graphics_draw_text(
        ctx,
        s_buffer,
        fonts_get_system_font(bold ? FONT_KEY_GOTHIC_18_BOLD : FONT_KEY_GOTHIC_18),
        GRect(cell.origin.x, cell.origin.y - FONT_OFFSET, cell.size.w, cell.size.h + FONT_OFFSET),
        GTextOverflowModeWordWrap,
        GTextAlignmentCenter,
        NULL);
}

static void calendar_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    int w = bounds.size.w;
    int h = bounds.size.h;
    float box_w = (float) w / DAYS_PER_WEEK;
    float box_h = (float) h / NUM_WEEKS;

    // Calculate which box holds today's date
    const int i_today = config_n_today();

    for (int i = 0; i < NUM_WEEKS * DAYS_PER_WEEK; ++i) {
        draw_day_cell(ctx, i, i_today, bounds, box_w, box_h);
    }
}

void calendar_layer_create(Layer* parent_layer, GRect frame) {
    memlog_heap("calendar_layer:create:start");
    s_calendar_layer = layer_create(frame);
    GRect bounds = layer_get_bounds(s_calendar_layer);
    int w = bounds.size.w;
    int h = bounds.size.h;
    float box_w = (float) w / DAYS_PER_WEEK;
    float box_h = (float) h / NUM_WEEKS;
    layer_set_update_proc(s_calendar_layer, calendar_update_proc);
    calendar_layer_refresh();
    layer_add_child(parent_layer, s_calendar_layer);
    memlog_heap("calendar_layer:create:end");
}


void calendar_layer_refresh() {
    memlog_heap("calendar_layer:refresh");
    // Request redraw (of today's highlight)
    layer_mark_dirty(s_calendar_layer);
}

void calendar_layer_destroy() {
    if (s_calendar_layer) {
        layer_destroy(s_calendar_layer);
        s_calendar_layer = NULL;
    }
    memlog_heap("calendar_layer:destroy");
}
