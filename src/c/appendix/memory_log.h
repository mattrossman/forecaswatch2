#pragma once

#include <pebble.h>

typedef struct
{
    const char *scope_tag;
    unsigned long min_free;
    unsigned long used_at_min;
} MemoryHeapProbe;

/*
 * MEMORY_LOG_HEAP(tag) records a one-off heap snapshot.
 *
 * Use probe helpers when you want to track heap behavior across one chunk of work.
 *
 *   MemoryHeapProbe probe = MEMORY_HEAP_PROBE_START("some_work");
 *   MEMORY_HEAP_PROBE_SAMPLE("step_1", &probe);
 *   MEMORY_HEAP_PROBE_SAMPLE("step_2", &probe);
 *   MEMORY_HEAP_PROBE_SAMPLE("step_3", &probe);
 *   MEMORY_HEAP_PROBE_LOG_MIN(&probe);
 *
 * Meaning:
 * - START: capture the starting heap snapshot for this work.
 * - SAMPLE: capture checkpoint snapshots while work is running.
 * - LOG_MIN: print the lowest free-heap point seen across this probe.
 *
 * The scope name is provided once at START and reused for the summary log.
 * SAMPLE logs are emitted as "scope:checkpoint", so checkpoint strings should
 * describe where the sample was taken.
 *
 * In release builds these probe macros compile to no-ops.
 */

#ifdef FCW2_ENABLE_MEMORY_LOGGING
static inline MemoryHeapProbe memory_heap_probe_start_impl(const char *scope_tag)
{
    MemoryHeapProbe probe = {
        .scope_tag = scope_tag,
        .min_free = (unsigned long)heap_bytes_free(),
        .used_at_min = (unsigned long)heap_bytes_used()};

    APP_LOG(APP_LOG_LEVEL_DEBUG, "MEM|%s:start|free=%lu|used=%lu",
            scope_tag,
            probe.min_free,
            probe.used_at_min);

    return probe;
}

static inline void memory_heap_probe_sample_impl(const char *checkpoint_tag, MemoryHeapProbe *probe)
{
    const unsigned long free_now = (unsigned long)heap_bytes_free();
    const unsigned long used_now = (unsigned long)heap_bytes_used();
    if (free_now < probe->min_free)
    {
        probe->min_free = free_now;
        probe->used_at_min = used_now;
    }

    APP_LOG(APP_LOG_LEVEL_DEBUG, "MEM|%s:%s|free=%lu|used=%lu",
            probe->scope_tag,
            checkpoint_tag,
            free_now,
            used_now);
}

static inline void memory_heap_probe_log_min_impl(const MemoryHeapProbe *probe)
{
    APP_LOG(APP_LOG_LEVEL_DEBUG, "MEM|%s:min_free|free=%lu|used=%lu",
            probe->scope_tag,
            probe->min_free,
            probe->used_at_min);
}

#define MEMORY_LOG_HEAP(tag)                                 \
    APP_LOG(APP_LOG_LEVEL_DEBUG, "MEM|%s|free=%lu|used=%lu", \
            tag,                                             \
            (unsigned long)heap_bytes_free(),                \
            (unsigned long)heap_bytes_used())
#define MEMORY_HEAP_PROBE_START(scope_tag) \
    memory_heap_probe_start_impl(scope_tag)
#define MEMORY_HEAP_PROBE_SAMPLE(tag, probe_ptr) \
    memory_heap_probe_sample_impl(tag, probe_ptr)
#define MEMORY_HEAP_PROBE_LOG_MIN(probe_ptr) \
    memory_heap_probe_log_min_impl(probe_ptr)
#else
#define MEMORY_LOG_HEAP(tag) \
    do                       \
    {                        \
    } while (0)
#define MEMORY_HEAP_PROBE_START(scope_tag) \
    ((void)(scope_tag), (MemoryHeapProbe){0, 0, 0})
#define MEMORY_HEAP_PROBE_SAMPLE(tag, probe_ptr) \
    do                                           \
    {                                            \
        (void)(tag);                             \
        (void)(probe_ptr);                       \
    } while (0)
#define MEMORY_HEAP_PROBE_LOG_MIN(probe_ptr) \
    do                                       \
    {                                        \
        (void)(probe_ptr);                   \
    } while (0)
#endif
