#include "weather_status_layer.h"
#include "c/appendix/persist.h"
#include "c/appendix/config.h"

#define FONT_18_OFFSET 7
#define FONT_14_OFFSET 3
#define CITY_INIT_WIDTH 100
#define ARROW_H 8
#define ARROW_HEAD_H 3
#define ARROW_HEAD_W 2
#define ARROW_W 6
#define ICON_W 8
#define MARGIN 2

static GRect frame_curr_temp;
static GRect frame_sun_event;

static bool s_show_precip = false;
static bool s_accel_tap_subscribed = false;

static bool should_show_precip() {
    switch (g_config->status_bar_mode) {
        case STATUS_BAR_MODE_SUN: return false;
        case STATUS_BAR_MODE_PRECIP: return true;
        default: return s_show_precip;  // STATUS_BAR_MODE_BOTH: use tap toggle
    }
}

static Layer *s_weather_status_layer;
static TextLayer *s_city_layer;
static TextLayer *s_current_temp_layer;
static TextLayer *s_next_sun_event_layer;

static GPath *s_arrow_path = NULL;
static const GPathInfo ARROW_PATH_INFO = {
    // Downward facing arrow, centered at the origin
    .num_points = 6,
    .points = (GPoint[]){
        {0, -ARROW_H/2},
        {0, ARROW_H/2 - ARROW_HEAD_H},
        {-ARROW_HEAD_W, ARROW_H/2 - ARROW_HEAD_H},
        {0, ARROW_H/2},
        {ARROW_HEAD_W, ARROW_H/2 - ARROW_HEAD_H},
        {0, ARROW_H/2 - ARROW_HEAD_H}
    }
};

static const GPathInfo RAINDROP_PATH_INFO = {
    // Teardrop: single-point apex, flares quickly, rounded belly
    .num_points = 6,
    .points = (GPoint[]){
        {0, -5}, {-3, 1}, {-2, 4}, {0, 5}, {2, 4}, {3, 1}
    }
};

static void text_layer_move_frame(TextLayer *text_layer, GRect frame) {
    layer_set_frame(text_layer_get_layer(text_layer), frame);
}

static void city_layer_refresh() {
    // Set the city text layer contents from storage
    static char s_city_buffer[20];
    persist_get_city(s_city_buffer, sizeof(s_city_buffer));
    text_layer_set_text(s_city_layer, s_city_buffer);

    // Dynamic resizing
    GRect bounds = layer_get_bounds(s_weather_status_layer);
    GSize size = text_layer_get_content_size(s_city_layer);
    int x = frame_curr_temp.origin.x + frame_curr_temp.size.w + MARGIN * 2;
    int y = -FONT_14_OFFSET;
    int w = bounds.size.w - frame_curr_temp.size.w - frame_sun_event.size.w - MARGIN * 4;
    int h = size.h + FONT_14_OFFSET;
    text_layer_move_frame(s_city_layer, GRect(x, y, w, h));
}

static void current_temp_layer_refresh() {
    static char s_temp_buffer[8];
    snprintf(s_temp_buffer, sizeof(s_temp_buffer), "• %d", config_localize_temp(persist_get_current_temp()));
    text_layer_set_text(s_current_temp_layer, s_temp_buffer);

    // Dynamic resizing
    text_layer_move_frame(s_current_temp_layer, GRect(0, 0, 100, 100));  // Make it big so content doesn't get clipped
    GSize size = text_layer_get_content_size(s_current_temp_layer);
    text_layer_move_frame(s_current_temp_layer, GRect(MARGIN, -FONT_18_OFFSET, size.w, size.h));
    frame_curr_temp = GRect(0, -FONT_18_OFFSET, size.w + MARGIN, size.h);
}

static void sun_event_layer_refresh() {
    GRect bounds = layer_get_bounds(s_weather_status_layer);

    static char s_buffer[12];

    if (should_show_precip()) {
        uint16_t tenths_mm = persist_get_precip_total();
        if (config_is_celsius()) {
            int mm_int = tenths_mm / 10;
            int mm_frac = tenths_mm % 10;
            snprintf(s_buffer, sizeof(s_buffer), "%d.%d mm", mm_int, mm_frac);
        } else {
            int hundredths_in = (tenths_mm * 100) / 254;
            int in_int = hundredths_in / 100;
            int in_frac = hundredths_in % 100;
            snprintf(s_buffer, sizeof(s_buffer), "%d.%02d\"", in_int, in_frac);
        }
    } else {
        // Get the time of the first sun event
        time_t first_sun_event_time;
        persist_get_sun_event_times(&first_sun_event_time, 1);
        struct tm *sun_time = localtime(&first_sun_event_time);
        config_format_time(s_buffer, sizeof(s_buffer), sun_time);
    }

    // Display this time on the TextLayer
    text_layer_set_text(s_next_sun_event_layer, s_buffer);

    // Dynamic resizing
    text_layer_move_frame(s_next_sun_event_layer, GRect(0, 0, 100, 100));  // Make it big so content doesn't get clipped
    GSize size = text_layer_get_content_size(s_next_sun_event_layer);
    if (should_show_precip()) {
        // Icon on left, text to its right
        int icon_w = (persist_get_precip_type() > 0) ? ICON_W + MARGIN : 0;
        int text_x = bounds.size.w - MARGIN - size.w;
        text_layer_move_frame(s_next_sun_event_layer, GRect(text_x, -FONT_14_OFFSET, size.w, size.h));
        frame_sun_event = GRect(text_x - icon_w, -FONT_14_OFFSET, size.w + icon_w + MARGIN, size.h);
    } else {
        // Arrow on left, text to its right
        int text_x = bounds.size.w - MARGIN - size.w;
        text_layer_move_frame(s_next_sun_event_layer, GRect(text_x, -FONT_14_OFFSET, size.w, size.h));
        frame_sun_event = GRect(text_x - ARROW_W - MARGIN, -FONT_14_OFFSET, size.w + ARROW_W + MARGIN * 2, size.h);
    }
}

static void weather_status_layer_init(GRect bounds) {
    // Set up the city text layer properties
    int w = bounds.size.w;

    // Current temperature
    s_current_temp_layer = text_layer_create(GRect(MARGIN, -FONT_18_OFFSET, 40, 25));
    text_layer_set_background_color(s_current_temp_layer, GColorClear);
    text_layer_set_text_alignment(s_current_temp_layer, GTextAlignmentLeft);
    text_layer_set_text_color(s_current_temp_layer, GColorWhite);
    text_layer_set_font(s_current_temp_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));

    // City where weather was fetched
    s_city_layer = text_layer_create(GRect(w/2 - CITY_INIT_WIDTH/2, -FONT_14_OFFSET, CITY_INIT_WIDTH, 25));
    text_layer_set_background_color(s_city_layer, GColorClear);
    text_layer_set_text_alignment(s_city_layer, GTextAlignmentCenter);
    text_layer_set_text_color(s_city_layer, GColorWhite);
    text_layer_set_font(s_city_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));

    // Time of next sun event (sunrise/sunset) or precip amount
    s_next_sun_event_layer = text_layer_create(GRect(w - MARGIN - 6 - 40, 4 - FONT_18_OFFSET, 40, 25));
    text_layer_set_background_color(s_next_sun_event_layer, GColorClear);
    text_layer_set_text_alignment(s_next_sun_event_layer, GTextAlignmentLeft);
    text_layer_set_text_color(s_next_sun_event_layer, GColorWhite);
    text_layer_set_font(s_next_sun_event_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));

    current_temp_layer_refresh();
    sun_event_layer_refresh();
    city_layer_refresh();
}

static void weather_status_update_proc(Layer *layer, GContext *ctx) {
    if (should_show_precip()) {
        uint8_t precip_type = persist_get_precip_type();
        if (precip_type > 0) {
            GColor icon_color = PBL_IF_COLOR_ELSE(
                precip_type == 2 ? GColorCeleste : GColorCobaltBlue,
                GColorWhite
            );
            int cx = frame_sun_event.origin.x + ICON_W / 2;
            int cy = 6;
            if (precip_type == 2) {
                // Snow: 6-armed asterisk (3 lines at 60° apart)
                graphics_context_set_stroke_color(ctx, icon_color);
                graphics_draw_line(ctx, GPoint(cx, cy - 4), GPoint(cx, cy + 4));
                graphics_draw_line(ctx, GPoint(cx - 3, cy - 2), GPoint(cx + 3, cy + 2));
                graphics_draw_line(ctx, GPoint(cx + 3, cy - 2), GPoint(cx - 3, cy + 2));
            } else {
                // Rain: teardrop GPath
                GPath *icon_path = gpath_create(&RAINDROP_PATH_INFO);
                gpath_move_to(icon_path, GPoint(cx, cy));
                graphics_context_set_fill_color(ctx, icon_color);
                gpath_draw_filled(ctx, icon_path);
                gpath_destroy(icon_path);
            }
        }
    } else {
        s_arrow_path = gpath_create(&ARROW_PATH_INFO);
        if (persist_get_sun_event_start_type() == 0)
            gpath_rotate_to(s_arrow_path, TRIG_MAX_ANGLE / 2);
        gpath_move_to(s_arrow_path, GPoint(frame_sun_event.origin.x + ARROW_W / 2, 6));
        graphics_context_set_stroke_color(ctx, GColorWhite);
        gpath_draw_outline_open(ctx, s_arrow_path);
        graphics_context_set_fill_color(ctx, GColorWhite);
        gpath_draw_filled(ctx, s_arrow_path);
        gpath_destroy(s_arrow_path);
    }
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
    (void) axis;
    (void) direction;
    s_show_precip = !s_show_precip;
    weather_status_layer_refresh();
}

static void update_accel_subscription() {
    bool should_subscribe = g_config->status_bar_mode == STATUS_BAR_MODE_BOTH;

    if (should_subscribe && !s_accel_tap_subscribed) {
        accel_tap_service_subscribe(tap_handler);
        s_accel_tap_subscribed = true;
    } else if (!should_subscribe && s_accel_tap_subscribed) {
        accel_tap_service_unsubscribe();
        s_accel_tap_subscribed = false;
    }
}

void weather_status_layer_create(Layer* parent_layer, GRect frame) {
    s_weather_status_layer = layer_create(frame);
    GRect bounds = layer_get_bounds(s_weather_status_layer);

    // Set up all the text layers
    weather_status_layer_init(bounds);
    layer_add_child(s_weather_status_layer, text_layer_get_layer(s_city_layer));
    layer_add_child(s_weather_status_layer, text_layer_get_layer(s_current_temp_layer));
    layer_add_child(s_weather_status_layer, text_layer_get_layer(s_next_sun_event_layer));
    layer_set_update_proc(s_weather_status_layer, weather_status_update_proc);

    update_accel_subscription();

    // Add the weather status bar to its parent
    layer_add_child(parent_layer, s_weather_status_layer);
}

void weather_status_layer_refresh() {
    update_accel_subscription();
    layer_mark_dirty(s_weather_status_layer);
    current_temp_layer_refresh();
    sun_event_layer_refresh();
    city_layer_refresh();
}

void weather_status_layer_destroy() {
    if (s_accel_tap_subscribed) {
        accel_tap_service_unsubscribe();
        s_accel_tap_subscribed = false;
    }
    text_layer_destroy(s_city_layer);
    text_layer_destroy(s_current_temp_layer);
    text_layer_destroy(s_next_sun_event_layer);
    layer_destroy(s_weather_status_layer);
}
