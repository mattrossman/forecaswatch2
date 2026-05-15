#include "watch_services.h"

time_t watch_services_now(void) {
#ifdef FCW2_MOCK_NOW_EPOCH
    return (time_t)FCW2_MOCK_NOW_EPOCH;
#else
    return time(NULL);
#endif
}
