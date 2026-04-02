#pragma once

#include <pebble.h>

#ifdef FCW2_ENABLE_MEMORY_LOGGING
#define MEMORY_LOG_HEAP(tag) \
    APP_LOG(APP_LOG_LEVEL_DEBUG, "MEM|%s|free=%lu|used=%lu", \
            tag, \
            (unsigned long)heap_bytes_free(), \
            (unsigned long)heap_bytes_used())
#else
#define MEMORY_LOG_HEAP(tag)
#endif
