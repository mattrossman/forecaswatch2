#include "config.h"
#include "persist.h"
#include "math.h"

int config_localize_temp(int temp_f) {
    // Convert temperatures as desired
    Config *config = (Config*) malloc(sizeof(Config));
    persist_get_config(config);
    int result;
    if (config->celsius)
        result = f_to_c(temp_f);
    else
        result = temp_f;
    free(config);
    return result;
}