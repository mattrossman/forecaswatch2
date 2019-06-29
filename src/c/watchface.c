#include <pebble.h>
#include "windows/main_window.h"
#include "appendix/app_message.h"
#include "appendix/persist.h"


static void init() {
    app_message_init();
    persist_init();
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
