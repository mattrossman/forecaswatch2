#pragma once

#include <pebble.h>
#include "config.h"

Config *g_config;

void globals_load();

void globals_unload();