#pragma once

#include <pebble.h>

void persist_init();

int persist_get_temp_lo();

int persist_get_temp_hi();

int persist_get_temp_trend(int16_t *buffer, const size_t buffer_size);

void persist_set_temp_lo(int val);

void persist_set_temp_hi(int val);

void persist_set_temp_trend(int16_t *data, const size_t size);