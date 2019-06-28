#pragma once

#include <pebble.h>

void persist_init();

int persist_get_temp_lo();

int persist_get_temp_hi();

int persist_get_temp_trend(int16_t *buffer, const size_t buffer_size);

int persist_get_precip_trend(uint8_t *buffer, const size_t buffer_size);

int persist_get_temp_start();

int persist_get_city(char *buffer, const size_t buffer_size);

void persist_set_temp_lo(int val);

void persist_set_temp_hi(int val);

void persist_set_temp_trend(int16_t *data, const size_t size);

void persist_set_precip_trend(uint8_t *data, const size_t size);

void persist_set_temp_start(int val);

void persist_set_city(char *val);