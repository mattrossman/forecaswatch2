#include "draw.h"

int16_t draw_hatch_start_y(int16_t x, int16_t y_start, int16_t spacing) {
    int16_t modulo = (x + y_start) % spacing;
    if (modulo < 0) {
        modulo += spacing;
    }

    if (modulo == 0) {
        return y_start;
    }

    return y_start + (spacing - modulo);
}

void draw_hatch_rect(GContext *ctx, GRect rect, int16_t spacing) {
    if (spacing <= 0 || rect.size.w <= 0 || rect.size.h <= 0) {
        return;
    }

    const int16_t x_end = rect.origin.x + rect.size.w;
    const int16_t y_end = rect.origin.y + rect.size.h;
    for (int16_t x = rect.origin.x; x < x_end; ++x) {
        int16_t hatch_y = draw_hatch_start_y(x, rect.origin.y, spacing);
        for (int16_t y = hatch_y; y < y_end; y += spacing) {
            graphics_draw_pixel(ctx, GPoint(x, y));
        }
    }
}
