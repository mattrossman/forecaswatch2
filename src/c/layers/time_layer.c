#include "time_layer.h"
#include "c/appendix/config.h"
#include "c/appendix/memory_log.h"
#include "c/services/watch_services.h"
#include <string.h>

// MT = Margin Top
#define MT_TIME 14
#define MT_AM_PM 7
#define MT_TIME_LECO 2
#define MT_AM_PM_LECO 2


static TextLayer *s_container_layer;
static TextLayer *s_time_layer;
static TextLayer *s_am_pm_layer;

/* emery: only its wider screen fits a date beside the time, and not next to
   its 60px Leco (the setting is greyed out for Leco). */
#ifdef PBL_PLATFORM_EMERY
// Gap between the time and the date block beside it
#define DATE_GAP 4
#define DATE_LINE_H 17

static Layer *s_date_layer;
// Weekday, then day and month; sized for multi-byte localized month names
static char s_date_lines[2][16];

static bool date_enabled(void) {
    return g_config->time_date_pos != TIME_DATE_OFF && g_config->time_font != TIME_FONT_LECO;
}

/* AM/PM takes the room the bold font needs. */
static GFont date_font(void) {
    return fonts_get_system_font(g_config->show_am_pm ? FONT_KEY_GOTHIC_14 : FONT_KEY_GOTHIC_18_BOLD);
}

static void date_update_proc(Layer *layer, GContext *ctx) {
    const GRect bounds = layer_get_bounds(layer);
    // Hug the time: right-align on its left side, left-align on its right side.
    const GTextAlignment alignment = g_config->time_date_pos == TIME_DATE_LEFT
            ? GTextAlignmentRight : GTextAlignmentLeft;

    graphics_context_set_text_color(ctx, g_config->color_time);
    for (int i = 0; i < 2; ++i) {
        graphics_draw_text(ctx, s_date_lines[i], date_font(),
                GRect(0, i * DATE_LINE_H, bounds.size.w, DATE_LINE_H + 8),
                GTextOverflowModeFill, alignment, NULL);
    }
}

static const char *ordinal_suffix(int day) {
    if (day >= 11 && day <= 13) {
        return "th";
    }
    switch (day % 10) {
        case 1: return "st";
        case 2: return "nd";
        case 3: return "rd";
        default: return "th";
    }
}

/* Day and month in the watch language's order: "Oct 18th" in English,
   "18. Okt" in German, "18 oct" elsewhere. */
static void format_day_month(char *buffer, size_t size, const struct tm *tm) {
    char month[12];
    strftime(month, sizeof(month), "%b", tm);
    const char *locale = i18n_get_system_locale();

    if (strncmp(locale, "en", 2) == 0) {
        snprintf(buffer, size, "%s %d%s", month, tm->tm_mday, ordinal_suffix(tm->tm_mday));
    } else if (strncmp(locale, "de", 2) == 0) {
        snprintf(buffer, size, "%d. %s", tm->tm_mday, month);
    } else {
        snprintf(buffer, size, "%d %s", tm->tm_mday, month);
    }
}

/* Fill the date lines and return the width of the wider one. */
static int date_prepare(const struct tm *tm) {
    strftime(s_date_lines[0], sizeof(s_date_lines[0]), "%a", tm);
    format_day_month(s_date_lines[1], sizeof(s_date_lines[1]), tm);

    int width = 0;
    for (int i = 0; i < 2; ++i) {
        const GSize size = graphics_text_layout_get_content_size(s_date_lines[i], date_font(),
                GRect(0, 0, 100, 100), GTextOverflowModeFill, GTextAlignmentLeft);
        if (size.w > width) {
            width = size.w;
        }
    }
    return width;
}
#endif

void time_layer_create(Layer* parent_layer, GRect frame) {
    s_container_layer = text_layer_create(frame);
    s_time_layer = text_layer_create(GRect(0, 0, frame.size.w, frame.size.h));
    s_am_pm_layer = text_layer_create(GRect(0, 0, 30, frame.size.h));

    text_layer_set_background_color(s_container_layer, GColorClear);

    // Main time formatting
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text(s_time_layer, "00:00");
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentLeft);

    // AM/PM formatting
    text_layer_set_font(s_am_pm_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_background_color(s_am_pm_layer, GColorClear);
    text_layer_set_text_color(s_am_pm_layer, GColorWhite);
    text_layer_set_text(s_am_pm_layer, "PM");
    text_layer_set_text_alignment(s_am_pm_layer, GTextAlignmentLeft);

    layer_add_child(text_layer_get_layer(s_container_layer), text_layer_get_layer(s_time_layer));
    // emery: date block beside the time, see date_prepare().
#ifdef PBL_PLATFORM_EMERY
    s_date_layer = layer_create(GRect(0, 0, 0, 0));
    layer_set_update_proc(s_date_layer, date_update_proc);
    layer_set_hidden(s_date_layer, true);
    layer_add_child(text_layer_get_layer(s_container_layer), s_date_layer);
#endif
    layer_add_child(text_layer_get_layer(s_time_layer), text_layer_get_layer(s_am_pm_layer));
    layer_add_child(parent_layer, text_layer_get_layer(s_container_layer));
    MEMORY_LOG_HEAP("after_time_layer_create");

}

// 12:30 -> 12:30
// 13:30 -> 1:30
// 00:30 -> 12:30

static void text_layer_move_frame(TextLayer *text_layer, GRect frame) {
    layer_set_frame(text_layer_get_layer(text_layer), frame);
}

void time_layer_tick() {
    // Get a tm structure
    struct tm tick_time = watch_services_localtime();

    // Format the time into a buffer
    static char s_buffer[8];
    config_format_time(s_buffer, 8, &tick_time);

    // Update the time and AM/PM indicator
    text_layer_set_text(s_time_layer, s_buffer);
    if (g_config->show_am_pm)
        text_layer_set_text(s_am_pm_layer, tick_time.tm_hour < 12 ? "AM" : "PM");
    
    // Reposition everything
    GRect bounds = layer_get_bounds(text_layer_get_layer(s_container_layer));
    text_layer_move_frame(s_time_layer, GRect(0, 0, bounds.size.w, bounds.size.h)); // Reset for size calculation
    GSize time_size = text_layer_get_content_size(s_time_layer);
    GSize am_pm_size = text_layer_get_content_size(s_am_pm_layer);

    // Calculate some landmarks
    int content_w = time_size.w + (g_config->show_am_pm ? am_pm_size.w : 0);
    int text_h = time_size.h - MT_TIME; // Remove top margin, approximately
    int text_top = -MT_TIME + (bounds.size.h/2 - text_h/2);
    int text_left = bounds.size.w / 2 - content_w / 2;

    // emery: center the time and the date beside it as one block.
#ifdef PBL_PLATFORM_EMERY
    const int date_w = date_enabled() ? date_prepare(&tick_time) : 0;
    const int block_left = bounds.size.w / 2 - (content_w + (date_w ? DATE_GAP + date_w : 0)) / 2;
    text_left = block_left
            + (date_w && g_config->time_date_pos == TIME_DATE_LEFT ? date_w + DATE_GAP : 0);
#endif

    // emery: nudge LECO time text upward slightly to keep optical centering.
#ifdef PBL_PLATFORM_EMERY
    if (g_config->time_font == TIME_FONT_LECO) {
        text_top -= MT_TIME_LECO;
    }
#endif

    // Update layer positions and visibility
    text_layer_move_frame(s_time_layer, GRect(text_left, text_top, content_w, time_size.h));
    if (g_config->show_am_pm) {
        int am_pm_y = MT_TIME - MT_AM_PM;
        // emery: nudge LECO AM/PM down slightly to align with larger time numerals.
#ifdef PBL_PLATFORM_EMERY
        if (g_config->time_font == TIME_FONT_LECO) {
            am_pm_y += MT_AM_PM_LECO;
        }
#endif
        text_layer_move_frame(s_am_pm_layer, GRect(time_size.w, am_pm_y, 30, time_size.h));
    }
    layer_set_hidden(text_layer_get_layer(s_am_pm_layer), !g_config->show_am_pm);

    // emery: place the date block on the chosen side of the time.
#ifdef PBL_PLATFORM_EMERY
    if (date_w) {
        const int date_h = 2 * DATE_LINE_H;
        const int date_left = g_config->time_date_pos == TIME_DATE_LEFT
                ? block_left : text_left + content_w + DATE_GAP;
        layer_set_frame(s_date_layer,
                GRect(date_left, (bounds.size.h - date_h) / 2 - 2, date_w, date_h + 6));
        layer_mark_dirty(s_date_layer);
    }
    layer_set_hidden(s_date_layer, !date_w);
#endif
}

void time_layer_refresh() {
    text_layer_set_font(s_time_layer, config_time_font());
    text_layer_set_text_color(s_time_layer, PBL_IF_COLOR_ELSE(g_config->color_time, GColorWhite));
    time_layer_tick();  // Update main time text and layer positions
}

void time_layer_destroy() {
    MEMORY_LOG_HEAP("time_layer_destroy:before");
    // emery: the date layer only exists there.
#ifdef PBL_PLATFORM_EMERY
    layer_destroy(s_date_layer);
#endif
    text_layer_destroy(s_time_layer);
    text_layer_destroy(s_container_layer);
    MEMORY_LOG_HEAP("time_layer_destroy:after");
}
