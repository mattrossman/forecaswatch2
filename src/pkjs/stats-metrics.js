/**
 * Stats grid metric ids.
 *
 * These numbers are a wire contract with the watch and MUST stay in lockstep
 * with StatMetricId in src/c/appendix/stats_metrics.h. Append only; never
 * renumber, because the values are persisted on the watch.
 */
var METRIC_IDS = {
    none: 0,
    steps: 1,
    distance: 2,
    calories: 3,
    active: 4,
    heartRate: 5,
    uvIndex: 6,
    battery: 7,
    currentTemp: 8,
    sunrise: 9,
    sunset: 10,
    date: 11,
    weekday: 12
};

/** Slots persisted on the watch; mirrors STATS_MAX_SLOTS. */
var MAX_SLOTS = 6;

/** Must match config_defaults() in src/c/appendix/config.c. */
var DEFAULT_SLOTS = ['distance', 'heartRate', 'steps', 'calories', 'active', 'uvIndex'];

/**
 * Resolve a slot setting to its wire id, falling back to the slot's default.
 *
 * @param {Object} settings Clay settings object.
 * @param {number} index Zero-based slot index.
 * @returns {number} Metric id for the watch.
 */
function slotId(settings, index) {
    var key = 'statSlot' + (index + 1);
    // Capability-gated Clay items are never built, so they never reach
    // serialize(); a missing key is normal rather than an error.
    var name = Object.prototype.hasOwnProperty.call(settings, key)
        ? settings[key]
        : DEFAULT_SLOTS[index];

    if (!Object.prototype.hasOwnProperty.call(METRIC_IDS, name)) {
        return METRIC_IDS[DEFAULT_SLOTS[index]];
    }

    return METRIC_IDS[name];
}

/**
 * Encode the top-band mode and every slot into one byte array.
 *
 * A single tuple is used because the watch's AppMessage inbox is 256 bytes and
 * already ~199 full; seven separate tuples would overflow it and the whole
 * message would be dropped. Byte 0 is the mode, bytes 1..MAX_SLOTS the metrics.
 *
 * @param {Object} settings Clay settings object.
 * @returns {Array<number>} Bytes for CLAY_STATS_SLOTS.
 */
function encodeSlots(settings) {
    var safe = settings || {};
    var bytes = [safe.topBand === 'stats' ? 1 : 0];

    for (var i = 0; i < MAX_SLOTS; i++) {
        bytes.push(slotId(safe, i));
    }

    return bytes;
}

module.exports = {
    METRIC_IDS: METRIC_IDS,
    DEFAULT_SLOTS: DEFAULT_SLOTS,
    encodeSlots: encodeSlots
};
