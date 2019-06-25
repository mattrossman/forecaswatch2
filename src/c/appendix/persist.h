#pragma once

#include <pebble.h>

void persist_init();

int persist_get_temp_lo();

int persist_get_temp_hi();

void persist_set_temp_lo(int val);

void persist_set_temp_hi(int val);