#include "globals.h"
#include "config.h"
#include "persist.h"

void globals_load() {
    g_config = (Config*) malloc(sizeof(Config));
    persist_get_config(g_config);
}

void globals_unload() {
    free(g_config);
}