#include "loading_layer.h"
#include "c/appendix/persist.h"

bool loading_layer_has_valid_data() {
    const time_t forecast_start = persist_get_forecast_start();
    const time_t now = time(NULL);

    return now - forecast_start <= 60 * 60 * 12;
}
