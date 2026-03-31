#pragma once

#include <pebble.h>

static inline void memlog_heap(const char *tag) {
    APP_LOG(APP_LOG_LEVEL_INFO, "[heap] %s free=%d", tag, (int)heap_bytes_free());
}
