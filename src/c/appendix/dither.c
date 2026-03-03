#include "dither.h"

// 8x8 Bayer threshold matrix (from Pebble's feature-frame-buffer example app).
static const uint8_t s_dither_matrix[8][8] = {
    {  0, 128,  32, 160,   8, 136,  40, 168},
    {192,  64, 224,  96, 200,  72, 232, 104},
    { 48, 176,  16, 144,  56, 184,  24, 152},
    {240, 112, 208,  80, 248, 120, 216,  88},
    { 12, 140,  44, 172,   4, 132,  36, 164},
    {204,  76, 236, 108, 196,  68, 228, 100},
    { 60, 188,  28, 156,  52, 180,  20, 148},
    {252, 124, 220,  92, 244, 116, 212,  84}
};

bool dither_is_white(int16_t x, int16_t y, uint8_t gray) {
    return gray > s_dither_matrix[y & 0x7][x & 0x7];
}

void dither_fill_rect_1bit_with_color(GContext *ctx, GRect rect, uint8_t gray, GColor color) {
    if (rect.size.w <= 0 || rect.size.h <= 0) {
        return;
    }

#if defined(PBL_COLOR)
    graphics_context_set_stroke_color(ctx, color);
#else
    graphics_context_set_stroke_color(ctx, GColorWhite);
#endif

    const int16_t x_end = rect.origin.x + rect.size.w;
    const int16_t y_end = rect.origin.y + rect.size.h;
    for (int16_t y = rect.origin.y; y < y_end; ++y) {
        for (int16_t x = rect.origin.x; x < x_end; ++x) {
            if (dither_is_white(x, y, gray)) {
                graphics_draw_pixel(ctx, GPoint(x, y));
            }
        }
    }
}

void dither_fill_rect_1bit(GContext *ctx, GRect rect, uint8_t gray) {
    dither_fill_rect_1bit_with_color(ctx, rect, gray, GColorWhite);
}
