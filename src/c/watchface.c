#include <pebble.h>
#include "windows/main_window.h"
#include "appendix/app_message.h"
#include "appendix/persist.h"
#include "appendix/config.h"
#include "appendix/memlog.h"


static void init() {
    memlog_heap("app:init:start");
    app_message_init();
    memlog_heap("after app_message_init");
    persist_init();
    memlog_heap("after persist_init");
    config_load();
    memlog_heap("after config_load");
    main_window_create();
    memlog_heap("after main_window_create");
}

static void deinit() {
    memlog_heap("app:deinit:start");
    config_unload();
    main_window_destroy();
    memlog_heap("app:deinit:end");
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
