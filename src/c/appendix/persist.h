#pragma once

#include <pebble.h>

#include "config.h"

void persist_init();

int persist_get_temp_lo();

int persist_get_temp_hi();

int persist_get_temp_trend(int16_t *buffer, const size_t buffer_size);

int persist_get_days_trend(int16_t *buffer, const size_t buffer_size);

int persist_get_days_icon(int16_t *buffer, const size_t buffer_size);

int persist_get_precip_days(uint8_t *buffer, const size_t buffer_size);

int persist_get_precip_trend(uint8_t *buffer, const size_t buffer_size);

int persist_get_windspeed_trend(uint8_t *buffer, const size_t buffer_size);

void persist_set_advice(int val);

void persist_set_holidays(int val);

int persist_get_advice();

int persist_get_holidays();

time_t persist_get_forecast_start();

int persist_get_num_entries();

int persist_get_num_days();

int persist_get_current_temp();

int persist_get_current_uvi();

int persist_get_city(char *buffer, const size_t buffer_size);

int persist_get_sun_event_start_type();

int persist_get_sun_event_times(time_t *buffer, const size_t buffer_size);

int persist_get_config(Config *config);

void persist_set_temp_lo(int val);

void persist_set_temp_hi(int val);

void persist_set_temp_trend(int16_t *data, const size_t size);

void persist_set_days_trend(int16_t *data, const size_t size);

void persist_set_days_icon(int16_t *data, const size_t size);

void persist_set_precip_days(uint8_t *data, const size_t size);

void persist_set_precip_trend(uint8_t *data, const size_t size);

void persist_set_windspeed_trend(uint8_t *data, const size_t size);

void persist_set_forecast_start(time_t val);

void persist_set_num_entries(int val);

void persist_set_num_days(int val);

void persist_set_current_temp(int val);

void persist_set_current_uvi(int val);

void persist_set_city(char *val);

void persist_set_sun_event_start_type(int val);

void persist_set_sun_event_times(time_t *data, const size_t size);

void persist_set_config(Config config);
