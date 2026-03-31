#include "loading_layer.h"
#include "c/appendix/persist.h"
#include "c/appendix/memlog.h"

static Layer *s_loading_layer;

static void loading_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    int w = bounds.size.w;
    int h = bounds.size.h;

    // Black out the weather components.
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(0, 0, w, h), 0, GCornerNone);

    // Draw directly instead of keeping another TextLayer alive.
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(
        ctx,
        "No data :(",
        fonts_get_system_font(FONT_KEY_GOTHIC_18),
        GRect(0, h / 3, w, h / 3),
        GTextOverflowModeWordWrap,
        GTextAlignmentCenter,
        NULL);
}

void loading_layer_create(Layer* parent_layer, GRect frame) {
    memlog_heap("loading_layer:create:start");
    s_loading_layer = layer_create(frame);
    if (!s_loading_layer) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create loading layer");
        return;
    }

    layer_set_update_proc(s_loading_layer, loading_update_proc);
    layer_add_child(parent_layer, s_loading_layer);
    memlog_heap("loading_layer:create:end");
}

void loading_layer_refresh() {
    memlog_heap("loading_layer:refresh:start");
    const time_t forecast_start = persist_get_forecast_start();
    const time_t now = time(NULL);
    if (now - forecast_start > 60 * 60 * 12) // 60 sec/min * 60 min/h * 12h
        layer_set_hidden(s_loading_layer, false); // show the no data notice
    else
        layer_set_hidden(s_loading_layer, true); // hide the no data notice
    memlog_heap("loading_layer:refresh:end");
}

void loading_layer_destroy() {
    if (s_loading_layer) {
        layer_destroy(s_loading_layer);
        s_loading_layer = NULL;
    }
    memlog_heap("loading_layer:destroy");
}
