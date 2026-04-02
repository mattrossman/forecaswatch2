#include <pebble.h>
#include "windows/main_window.h"
#include "appendix/app_message.h"
#include "appendix/persist.h"
#include "appendix/config.h"
#include "appendix/memory_log.h"


static void init() {
    memory_log_heap("boot");
    app_message_init();
    persist_init();
    config_load();
    main_window_create();
    memory_log_heap("after_main_window_create");
}

static void deinit() {
    memory_log_heap("before_teardown");
    config_unload();
    main_window_destroy();
    memory_log_heap("after_teardown");
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
