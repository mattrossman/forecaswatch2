#include <pebble.h>
#include "windows/main_window.h"
#include "appendix/define_constants.h"

static void init() {
    main_window_create();
}

static void deinit() {
    main_window_destroy();
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
