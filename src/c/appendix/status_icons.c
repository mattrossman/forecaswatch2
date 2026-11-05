#include "status_icons.h"
#include "config.h"

static GBitmap *s_mute_bitmap;
static GBitmap *s_bt_bitmap;
static GBitmap *s_bt_disconnect_bitmap;
/* Palettes must outlive the bitmaps: gbitmap_set_palette(..., false) does not
   take ownership. */
static GColor s_mute_palette[2];
static GColor s_bt_palette[2];
static GColor s_bt_disconnect_palette[2];

typedef struct {
    bool qt;
    bool bt;
    bool bt_disconnect;
} StatusIconVisibility;

static StatusIconVisibility status_icons_visibility() {
    bool connected = connection_service_peek_pebble_app_connection();
    return (StatusIconVisibility) {
        .qt = status_icons_show_qt(),
        .bt = connected && g_config->show_bt,
        .bt_disconnect = !connected && g_config->show_bt_disconnect
    };
}

static void draw_bitmap(GContext *ctx, GBitmap *bitmap, GRect frame) {
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_bitmap_in_rect(ctx, bitmap, frame);
    graphics_context_set_compositing_mode(ctx, GCompOpAssign);
}

static void ensure_mute_bitmap_loaded(void) {
    if (!s_mute_bitmap) {
        s_mute_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MUTE);
        s_mute_palette[0] = GColorWhite;
        s_mute_palette[1] = GColorClear;
        gbitmap_set_palette(s_mute_bitmap, s_mute_palette, false);
    }
}

static void ensure_bt_bitmap_loaded(void) {
    if (!s_bt_bitmap) {
        s_bt_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_CONNECT);
        s_bt_palette[0] = PBL_IF_COLOR_ELSE(GColorPictonBlue, GColorWhite);
        s_bt_palette[1] = GColorClear;
        gbitmap_set_palette(s_bt_bitmap, s_bt_palette, false);
    }
}

static void ensure_bt_disconnect_bitmap_loaded(void) {
    if (!s_bt_disconnect_bitmap) {
        s_bt_disconnect_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_DISCONNECT);
        s_bt_disconnect_palette[0] = PBL_IF_COLOR_ELSE(GColorRed, GColorWhite);
        s_bt_disconnect_palette[1] = GColorClear;
        gbitmap_set_palette(s_bt_disconnect_bitmap, s_bt_disconnect_palette, false);
    }
}

static void maybe_unload_bitmaps(StatusIconVisibility visible) {
    if (!visible.qt && s_mute_bitmap) {
        gbitmap_destroy(s_mute_bitmap);
        s_mute_bitmap = NULL;
    }

    if (!visible.bt && s_bt_bitmap) {
        gbitmap_destroy(s_bt_bitmap);
        s_bt_bitmap = NULL;
    }

    if (!visible.bt_disconnect && s_bt_disconnect_bitmap) {
        gbitmap_destroy(s_bt_disconnect_bitmap);
        s_bt_disconnect_bitmap = NULL;
    }
}

static int visible_icon_count(StatusIconVisibility visible) {
    return (visible.qt ? 1 : 0) + ((visible.bt || visible.bt_disconnect) ? 1 : 0);
}

static int icon_row_width(int count, int spacing) {
    if (count <= 0) {
        return 0;
    }

    return count * STATUS_ICON_SIZE + (count - 1) * spacing;
}

bool status_icons_show_qt(void) {
    return g_config->show_qt && quiet_time_is_active();
}

int status_icons_draw(GContext *ctx, GPoint origin, int spacing) {
    StatusIconVisibility visible = status_icons_visibility();

    maybe_unload_bitmaps(visible);

    int x = origin.x;

    if (visible.qt) {
        ensure_mute_bitmap_loaded();
        draw_bitmap(ctx, s_mute_bitmap, GRect(x, origin.y, STATUS_ICON_SIZE, STATUS_ICON_SIZE));
        x += STATUS_ICON_SIZE + spacing;
    }

    if (visible.bt) {
        ensure_bt_bitmap_loaded();
        draw_bitmap(ctx, s_bt_bitmap, GRect(x, origin.y, STATUS_ICON_SIZE, STATUS_ICON_SIZE));
    } else if (visible.bt_disconnect) {
        ensure_bt_disconnect_bitmap_loaded();
        draw_bitmap(ctx, s_bt_disconnect_bitmap, GRect(x, origin.y, STATUS_ICON_SIZE, STATUS_ICON_SIZE));
    }

    return icon_row_width(visible_icon_count(visible), spacing);
}

void status_icons_unload(void) {
    maybe_unload_bitmaps((StatusIconVisibility) { false, false, false });
}
