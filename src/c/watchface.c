#include <pebble.h>
#include "windows/main_window.h"
#include "appendix/app_message.h"
#include "appendix/persist.h"
#include "appendix/config.h"
#include "appendix/memory_log.h"


static void init() {
    MEMORY_LOG_HEAP("boot");
    app_message_init();
    persist_init();
    config_load();
    main_window_create();
    MEMORY_LOG_HEAP("after_main_window_create");
}

static void deinit() {
    MEMORY_LOG_HEAP("before_teardown");
    config_unload();
    main_window_destroy();
    MEMORY_LOG_HEAP("after_teardown");
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
