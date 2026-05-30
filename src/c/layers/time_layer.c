#include "time_layer.h"
#include "c/appendix/config.h"
#include "c/appendix/memory_log.h"
#include "c/services/watch_services.h"

// MT = Margin Top
#define MT_TIME 14
#define MT_AM_PM 7
#define MT_TIME_LECO 2
#define MT_AM_PM_LECO 2
#define AM_PM_GAP 5
#define HEALTH_RULE_DRAW_W 15
#define HEALTH_STACK_GAP 4
#define HEALTH_FONT_TOP_OFFSET 3
#define HEALTH_FONT_KEY FONT_KEY_GOTHIC_18

#ifdef PBL_PLATFORM_EMERY
#define AM_PM_FONT_KEY FONT_KEY_GOTHIC_14
#else
#define AM_PM_FONT_KEY FONT_KEY_GOTHIC_14
#endif


static Layer *s_container_layer;
static TextLayer *s_time_layer;
static TextLayer *s_am_pm_layer;
static TextLayer *s_heart_rate_layer;
static TextLayer *s_steps_layer;
static Layer *s_health_rule_layer;
#ifndef PBL_PLATFORM_APLITE
static bool s_health_subscribed;
#endif

static void text_layer_move_frame(TextLayer *text_layer, GRect frame) {
    layer_set_frame(text_layer_get_layer(text_layer), frame);
}

static bool should_show_health_stats(void) {
    return g_config->show_heart_rate || g_config->show_steps;
}

static GFont time_layer_compact_font(void) {
    switch (g_config->time_font) {
        case TIME_FONT_LECO:
#ifdef PBL_PLATFORM_EMERY
            return fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
#else
            return fonts_get_system_font(FONT_KEY_LECO_38_BOLD_NUMBERS);
#endif
        case TIME_FONT_BITHAM:
            return fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS);
        case TIME_FONT_ROBOTO:
        default:
            // No smaller Roboto numeral font; Bitham 42 keeps the row compact.
            return fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS);
    }
}

#ifndef PBL_PLATFORM_APLITE
static bool health_metric_available(HealthMetric metric, bool instantaneous) {
    time_t now = time(NULL);
    time_t start = instantaneous ? now : time_start_of_today();
    return (health_service_metric_accessible(metric, start, now) & HealthServiceAccessibilityMaskAvailable) != 0;
}

static void health_event_handler(HealthEventType event, void *context) {
    if (event == HealthEventMovementUpdate || event == HealthEventHeartRateUpdate) {
        time_layer_tick();
    }
}

static void ensure_health_subscription(void) {
    bool want_subscription = should_show_health_stats();
    if (want_subscription && !s_health_subscribed) {
        if (health_service_events_subscribe(health_event_handler, NULL)) {
            s_health_subscribed = true;
        }
    } else if (!want_subscription && s_health_subscribed) {
        health_service_events_unsubscribe();
        s_health_subscribed = false;
    }
}
#endif

static void health_rule_update_proc(Layer *layer, GContext *ctx) {
    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(g_config->color_time, GColorWhite));
    GRect bounds = layer_get_bounds(layer);
    int x = (bounds.size.w - HEALTH_RULE_DRAW_W) / 2;
    graphics_draw_line(ctx, GPoint(x, 0), GPoint(x + HEALTH_RULE_DRAW_W, 0));
}

static void refresh_health_text(void) {
    if (!should_show_health_stats()) {
        return;
    }

    static char heart_rate_buffer[8];
    static char steps_buffer[8];

    if (g_config->show_heart_rate) {
        PBL_IF_HEALTH_ELSE({
            if (health_metric_available(HealthMetricHeartRateBPM, true)) {
                snprintf(heart_rate_buffer, sizeof(heart_rate_buffer), "%d",
                        (int) health_service_peek_current_value(HealthMetricHeartRateBPM));
            } else {
                snprintf(heart_rate_buffer, sizeof(heart_rate_buffer), "--");
            }
        }, {
            snprintf(heart_rate_buffer, sizeof(heart_rate_buffer), "--");
        });
        text_layer_set_text(s_heart_rate_layer, heart_rate_buffer);
    }

    if (g_config->show_steps) {
        PBL_IF_HEALTH_ELSE({
            if (health_metric_available(HealthMetricStepCount, false)) {
                snprintf(steps_buffer, sizeof(steps_buffer), "%d",
                        (int) health_service_sum_today(HealthMetricStepCount));
            } else {
                snprintf(steps_buffer, sizeof(steps_buffer), "--");
            }
        }, {
            snprintf(steps_buffer, sizeof(steps_buffer), "--");
        });
        text_layer_set_text(s_steps_layer, steps_buffer);
    }
}

static int measure_health_column(GRect bounds, GSize *heart_rate_size, GSize *steps_size, int *health_line_h) {
    const bool show_heart_rate = g_config->show_heart_rate;
    const bool show_steps = g_config->show_steps;
    GFont health_font = fonts_get_system_font(HEALTH_FONT_KEY);

    *heart_rate_size = GSizeZero;
    *steps_size = GSizeZero;
    *health_line_h = 0;

    if (show_heart_rate) {
        text_layer_set_font(s_heart_rate_layer, health_font);
        text_layer_move_frame(s_heart_rate_layer, GRect(0, 0, bounds.size.w, bounds.size.h));
        layer_set_hidden(text_layer_get_layer(s_heart_rate_layer), false);
        *heart_rate_size = text_layer_get_content_size(s_heart_rate_layer);
        *health_line_h = heart_rate_size->h;
    }

    if (show_steps) {
        text_layer_set_font(s_steps_layer, health_font);
        text_layer_move_frame(s_steps_layer, GRect(0, 0, bounds.size.w, bounds.size.h));
        layer_set_hidden(text_layer_get_layer(s_steps_layer), false);
        *steps_size = text_layer_get_content_size(s_steps_layer);
        if (steps_size->h > *health_line_h) {
            *health_line_h = steps_size->h;
        }
    }

    int column_w = heart_rate_size->w;
    if (steps_size->w > column_w) {
        column_w = steps_size->w;
    }
    if (HEALTH_RULE_DRAW_W > column_w) {
        column_w = HEALTH_RULE_DRAW_W;
    }
    return column_w;
}

static void layout_health_text_row(TextLayer *layer, int x, int y, int column_w, int health_line_h) {
    text_layer_move_frame(layer, GRect(x, y - HEALTH_FONT_TOP_OFFSET, column_w, health_line_h + HEALTH_FONT_TOP_OFFSET));
}

static void layout_health_stats(GRect bounds, int column_w, int health_line_h) {
    layer_set_hidden(s_health_rule_layer, true);
    layer_set_hidden(text_layer_get_layer(s_heart_rate_layer), true);
    layer_set_hidden(text_layer_get_layer(s_steps_layer), true);

    const bool show_heart_rate = g_config->show_heart_rate;
    const bool show_steps = g_config->show_steps;

    int stack_h = 0;
    if (show_heart_rate) {
        stack_h += health_line_h;
    }
    if (show_heart_rate && show_steps) {
        stack_h += HEALTH_STACK_GAP + 1 + HEALTH_STACK_GAP;
    }
    if (show_steps) {
        stack_h += health_line_h;
    }

    int health_left = bounds.size.w - column_w;

    int y = (bounds.size.h - stack_h) / 2;
    y -= 1.25;

    if (show_heart_rate) {
        layout_health_text_row(s_heart_rate_layer, health_left, y, column_w, health_line_h);
        layer_set_hidden(text_layer_get_layer(s_heart_rate_layer), false);
        y += health_line_h + HEALTH_STACK_GAP;
    }

    if (show_heart_rate && show_steps) {
        layer_set_frame(s_health_rule_layer, GRect(health_left, y, column_w, 1));
        layer_set_hidden(s_health_rule_layer, false);
        layer_mark_dirty(s_health_rule_layer);
        y += 1 + HEALTH_STACK_GAP;
    }

    if (show_steps) {
        layout_health_text_row(s_steps_layer, health_left, y, column_w, health_line_h);
        layer_set_hidden(text_layer_get_layer(s_steps_layer), false);
    }
}

static void hide_health_stats(void) {
    layer_set_hidden(s_health_rule_layer, true);
    layer_set_hidden(text_layer_get_layer(s_heart_rate_layer), true);
    layer_set_hidden(text_layer_get_layer(s_steps_layer), true);
}

static void layout_time_without_health(GRect bounds, GSize time_size, GSize am_pm_size) {
    int content_w = time_size.w + (g_config->show_am_pm ? am_pm_size.w : 0);
    int text_h = time_size.h - MT_TIME;
    int text_top = -MT_TIME + (bounds.size.h / 2 - text_h / 2) - 2;
    int text_left = bounds.size.w / 2 - content_w / 2;

#ifdef PBL_PLATFORM_EMERY
    if (g_config->time_font == TIME_FONT_LECO) {
        text_top -= MT_TIME_LECO;
    }
#endif

    text_layer_move_frame(s_time_layer, GRect(text_left, text_top, content_w, time_size.h));
    if (g_config->show_am_pm) {
        int am_pm_y = MT_TIME - MT_AM_PM;
#ifdef PBL_PLATFORM_EMERY
        if (g_config->time_font == TIME_FONT_LECO) {
            am_pm_y += MT_AM_PM_LECO;
        }
#endif
        text_layer_move_frame(s_am_pm_layer, GRect(text_left + time_size.w, text_top + am_pm_y, 30, time_size.h));
    }
}

static void layout_time_with_health(GRect bounds, GSize time_size, GSize am_pm_size, int health_col_w, int health_line_h) {
    const int time_block_w = time_size.w + (g_config->show_am_pm ? AM_PM_GAP + am_pm_size.w : 0);
    int text_h = time_size.h - MT_TIME;
    int text_top = -MT_TIME + (bounds.size.h / 2 - text_h / 2);
    int text_left = (bounds.size.w / 2 - time_block_w / 2) + 4;
    if (text_left < 0) {
        text_left = 0;
    }

#ifdef PBL_PLATFORM_EMERY
    if (g_config->time_font == TIME_FONT_LECO) {
        text_top -= MT_TIME_LECO;
    }
#endif

    text_layer_move_frame(s_time_layer, GRect(text_left, text_top, time_size.w, time_size.h));
    if (g_config->show_am_pm) {
        int am_pm_y = text_top + 8;
        text_layer_move_frame(s_am_pm_layer, GRect(text_left + time_size.w + AM_PM_GAP, am_pm_y, am_pm_size.w, am_pm_size.h));
    }

    layout_health_stats(bounds, health_col_w, health_line_h);
}

void time_layer_create(Layer* parent_layer, GRect frame) {
    s_container_layer = layer_create(frame);
    s_time_layer = text_layer_create(GRect(0, 0, frame.size.w, frame.size.h));
    s_am_pm_layer = text_layer_create(GRect(0, 0, 30, frame.size.h));
    s_heart_rate_layer = text_layer_create(GRect(0, 0, frame.size.w, frame.size.h));
    s_steps_layer = text_layer_create(GRect(0, 0, frame.size.w, frame.size.h));
    s_health_rule_layer = layer_create(GRect(0, 0, HEALTH_RULE_DRAW_W, 1));
    layer_set_update_proc(s_health_rule_layer, health_rule_update_proc);

    // Main time formatting
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text(s_time_layer, "00:00");
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentLeft);

    // AM/PM formatting (match city/location font size)
    text_layer_set_font(s_am_pm_layer, fonts_get_system_font(AM_PM_FONT_KEY));
    text_layer_set_background_color(s_am_pm_layer, GColorClear);
    text_layer_set_text_color(s_am_pm_layer, GColorWhite);
    text_layer_set_text(s_am_pm_layer, "PM");
    text_layer_set_text_alignment(s_am_pm_layer, GTextAlignmentLeft);

    // Health stats formatting
    GFont health_font = fonts_get_system_font(HEALTH_FONT_KEY);
    text_layer_set_font(s_heart_rate_layer, health_font);
    text_layer_set_font(s_steps_layer, health_font);
    text_layer_set_background_color(s_heart_rate_layer, GColorClear);
    text_layer_set_text_color(s_heart_rate_layer, GColorWhite);
    text_layer_set_text(s_heart_rate_layer, "--");
    text_layer_set_text_alignment(s_heart_rate_layer, GTextAlignmentCenter);

    text_layer_set_background_color(s_steps_layer, GColorClear);
    text_layer_set_text_color(s_steps_layer, GColorWhite);
    text_layer_set_text(s_steps_layer, "--");
    text_layer_set_text_alignment(s_steps_layer, GTextAlignmentCenter);

    layer_add_child(s_container_layer, text_layer_get_layer(s_time_layer));
    layer_add_child(s_container_layer, text_layer_get_layer(s_am_pm_layer));
    layer_add_child(s_container_layer, text_layer_get_layer(s_heart_rate_layer));
    layer_add_child(s_container_layer, text_layer_get_layer(s_steps_layer));
    layer_add_child(s_container_layer, s_health_rule_layer);
    layer_add_child(parent_layer, s_container_layer);
    layer_set_hidden(s_health_rule_layer, true);
    layer_set_hidden(text_layer_get_layer(s_heart_rate_layer), true);
    layer_set_hidden(text_layer_get_layer(s_steps_layer), true);
    MEMORY_LOG_HEAP("after_time_layer_create");
}

// 12:30 -> 12:30
// 13:30 -> 1:30
// 00:30 -> 12:30

void time_layer_tick() {
    struct tm tick_time = watch_services_localtime();

    static char s_buffer[8];
    config_format_time(s_buffer, 8, &tick_time);

    const bool show_health = should_show_health_stats();
    text_layer_set_font(s_time_layer, show_health ? time_layer_compact_font() : config_time_font());
    text_layer_set_text(s_time_layer, s_buffer);
    if (g_config->show_am_pm) {
        text_layer_set_text(s_am_pm_layer, tick_time.tm_hour < 12 ? "AM" : "PM");
    }

    GRect bounds = layer_get_bounds(s_container_layer);
    text_layer_move_frame(s_time_layer, GRect(0, 0, bounds.size.w, bounds.size.h));
    GSize time_size = text_layer_get_content_size(s_time_layer);
    GSize am_pm_size = text_layer_get_content_size(s_am_pm_layer);

    if (show_health) {
        refresh_health_text();
        int health_col_w = 0;
        int health_line_h = 0;
        GSize heart_rate_size = GSizeZero;
        GSize steps_size = GSizeZero;
        health_col_w = measure_health_column(bounds, &heart_rate_size, &steps_size, &health_line_h);
        layout_time_with_health(bounds, time_size, am_pm_size, health_col_w, health_line_h);
    } else {
        hide_health_stats();
        layout_time_without_health(bounds, time_size, am_pm_size);
    }

    layer_set_hidden(text_layer_get_layer(s_am_pm_layer), !g_config->show_am_pm);
}

void time_layer_refresh() {
    if (should_show_health_stats()) {
        text_layer_set_font(s_time_layer, time_layer_compact_font());
        text_layer_set_font(s_am_pm_layer, fonts_get_system_font(AM_PM_FONT_KEY));
    } else {
        text_layer_set_font(s_time_layer, config_time_font());
        text_layer_set_font(s_am_pm_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    }
    text_layer_set_text_color(s_time_layer, PBL_IF_COLOR_ELSE(g_config->color_time, GColorWhite));
    text_layer_set_text_color(s_heart_rate_layer, PBL_IF_COLOR_ELSE(g_config->color_time, GColorWhite));
    text_layer_set_text_color(s_steps_layer, PBL_IF_COLOR_ELSE(g_config->color_time, GColorWhite));
    PBL_IF_HEALTH_ELSE(ensure_health_subscription(), {});
    time_layer_tick();
}

void time_layer_destroy() {
    MEMORY_LOG_HEAP("time_layer_destroy:before");
    PBL_IF_HEALTH_ELSE({
        if (s_health_subscribed) {
            health_service_events_unsubscribe();
            s_health_subscribed = false;
        }
    }, {});
    text_layer_destroy(s_am_pm_layer);
    text_layer_destroy(s_heart_rate_layer);
    text_layer_destroy(s_steps_layer);
    layer_destroy(s_health_rule_layer);
    text_layer_destroy(s_time_layer);
    layer_destroy(s_container_layer);
    MEMORY_LOG_HEAP("time_layer_destroy:after");
}
