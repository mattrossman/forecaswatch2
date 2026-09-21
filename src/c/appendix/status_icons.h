#pragma once

#include <pebble.h>

#define STATUS_ICON_SIZE 10

/* Quiet-time and bluetooth indicator icons, shared by the calendar status bar
   and the stats grid. Bitmaps are lazily created on first draw and destroyed as
   soon as their icon stops being visible, so only what is on screen holds heap. */

bool status_icons_show_qt(void);

/* Draw the visible icons left to right, `origin` being the top-left of the
   first one. Returns the width consumed. */
int status_icons_draw(GContext *ctx, GPoint origin, int spacing);

void status_icons_unload(void);
