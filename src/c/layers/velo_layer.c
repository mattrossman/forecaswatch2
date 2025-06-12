#include "velo_layer.h"
#include "c/appendix/persist.h"
#include "c/appendix/math.h"
#include "c/appendix/config.h"

#define LEFT_AXIS_MARGIN_W 17
#define BOTTOM_AXIS_FONT_OFFSET 4  // Adjustment for whitespace at top of font
#define LABEL_PADDING 20  // Minimum width a label should cover
#define BOTTOM_AXIS_H 10  // Height of the bottom axis (hour labels)
#define MARGIN_TEMP_H 7  // Height of margins for the temperature plot
#define NUM_WEEKS 1
#define DAYS_PER_WEEK 7
#define FONT_OFFSET 5

static Layer *s_velo_layer;
static TextLayer *s_calendar_text_layers[NUM_WEEKS * DAYS_PER_WEEK];
static TextLayer *s_advice_text_layer;

static void velo_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);

    // Load data from storage
    const int num_days = persist_get_num_days();
    const int advice = persist_get_advice();
    int16_t temps[num_days];
    int16_t icons[num_days];
    uint8_t precips[num_days];
    static char s_calendar_box_buffers[NUM_WEEKS * DAYS_PER_WEEK][4];
    static char s_advice_box_buffer[20];

    persist_get_days_trend(temps, num_days);
    persist_get_days_icon(icons, num_days);
    persist_get_precip_days(precips, num_days);

    // Fill each box with an appropriate relative day number
    for (int i = 0; i < NUM_WEEKS * DAYS_PER_WEEK; ++i) {
        char *buffer = s_calendar_box_buffers[i];
        
        GColor text_color = GColorWhite;
        if (precips[i]>50) {
            text_color = GColorBlue;
        }
        text_layer_set_text_color(s_calendar_text_layers[i], text_color);

        // Use bold font for today, and holidays/weekends if colored
        bool bold = true;
        text_layer_set_font(s_calendar_text_layers[i],
            fonts_get_system_font(bold ? FONT_KEY_GOTHIC_18_BOLD : FONT_KEY_GOTHIC_18));

        snprintf(buffer, 4, "%d", config_localize_temp(temps[i]));  
        text_layer_set_text(s_calendar_text_layers[i], buffer);
    }
        
    {    
        GColor text_color = GColorWhite;
        text_layer_set_text_color(s_advice_text_layer, text_color);

        text_layer_set_font(s_advice_text_layer,fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));

        switch (advice)
        {
            case 1:
                    snprintf(s_advice_box_buffer, 19, "%s", "Need more training!");
                    break;
            case 2:
                    snprintf(s_advice_box_buffer, 19, "%s", "Fresh condition!");
                    break;
            case 3:
                    snprintf(s_advice_box_buffer, 19, "%s", "Keeping it steady!");
                    break;
            case 4:
                    snprintf(s_advice_box_buffer, 19, "%s", "Improving!");
                    break;
            case 5:
                    snprintf(s_advice_box_buffer, 19, "%s", "Slightly overtrained!");
                    break;
            case 6:
                    snprintf(s_advice_box_buffer, 19, "%s", "Overtraining warning!");
                    break;
            case 7:
                    snprintf(s_advice_box_buffer, 19, "%s", "No RiDuck data!");
                    break;          
            default:
                    snprintf(s_advice_box_buffer, 19, "%d", advice);
                    break;
        } 
        text_layer_set_text(s_advice_text_layer, s_advice_box_buffer);
    }

}

void velo_layer_create(Layer *parent_layer, GRect frame) {
    s_velo_layer = layer_create(frame);
    GRect bounds = layer_get_bounds(s_velo_layer);

    int w = bounds.size.w;
    int h = bounds.size.h-18;
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
        layer_add_child(s_velo_layer, text_layer_get_layer(s_box_text_layer));
    }
    {
        s_advice_text_layer = text_layer_create(
            GRect(0, 1 * box_h - FONT_OFFSET -3,
                  w, box_h + FONT_OFFSET));
        text_layer_set_background_color(s_advice_text_layer, GColorClear);
        text_layer_set_text_alignment(s_advice_text_layer, GTextAlignmentCenter);
        layer_add_child(s_velo_layer, text_layer_get_layer(s_advice_text_layer));   
    }
    layer_set_update_proc(s_velo_layer, velo_update_proc);

    // Add it as a child layer to the Window's root layer
    layer_add_child(parent_layer, s_velo_layer);
}

void velo_layer_refresh() {
    layer_mark_dirty(s_velo_layer);
}

void velo_layer_destroy() {
    for (int i = 0; i < NUM_WEEKS * DAYS_PER_WEEK; ++i) {
        text_layer_destroy(s_calendar_text_layers[i]);
    }
    text_layer_destroy(s_advice_text_layer);
    layer_destroy(s_velo_layer);
}