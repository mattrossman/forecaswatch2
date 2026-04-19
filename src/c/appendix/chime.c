#include "chime.h"
#include "config.h"

void chime_handle(struct tm *tick_time) {
  if (g_config->chime == 30 && tick_time->tm_min == 30) {
    vibes_short_pulse();
  } else if (g_config->chime == 60 && tick_time->tm_min == 0) {
    vibes_double_pulse();
  }
}
