#include "main_window.h"
#include "../layers/time_layer.h"

static Window *s_main_window;

static void main_window_load(Window *window) {
    // Get information about the Window
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    // Create the TextLayer with specific bounds
    time_layer_create(window_layer,
            GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));
}

static void main_window_unload(Window *window) {
    time_layer_destroy();
}

/*----------------------------
-------- EXTERNAL ------------
----------------------------*/

void main_window_create() {
    // Create main Window element and assign to pointer
    s_main_window = window_create();

    // Set handlers to manage the elements inside the Window
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = main_window_load,
        .unload = main_window_unload
    });

    // Show the window on the watch with animated=true
    window_stack_push(s_main_window, true);
}

void main_window_destroy() {
    // Interface for destroying the main window (implicitly unloads contents)
    window_destroy(s_main_window);
}