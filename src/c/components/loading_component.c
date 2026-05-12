#include "loading_component.h"

void loading_component_render(GContext *ctx, GRect bounds) {
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, "No data :(",
                       fonts_get_system_font(FONT_KEY_GOTHIC_18),
                       GRect(0, bounds.size.h / 3, bounds.size.w, bounds.size.h),
                       GTextOverflowModeWordWrap,
                       GTextAlignmentCenter,
                       NULL);
}
