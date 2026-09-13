var statsMetrics = require('../stats-metrics.js');

/* Metrics offered for a slot. Keys match stats-metrics.js METRIC_IDS.
   Sorted alphabetically below rather than by hand, so metrics added later land
   in the right place without anyone having to re-order this list. */
var METRIC_OPTIONS = [
    { label: 'Steps', value: 'steps' },
    { label: 'Distance', value: 'distance' },
    { label: 'Calories', value: 'calories' },
    { label: 'Activity', value: 'active' },
    { label: 'Heart rate', value: 'heartRate' },
    { label: 'UV index', value: 'uvIndex' },
    { label: 'Battery', value: 'battery' },
    { label: 'Temperature', value: 'currentTemp' },
    { label: 'Sunrise', value: 'sunrise' },
    { label: 'Sunset', value: 'sunset' },
    { label: 'Date', value: 'date' },
    { label: 'Weekday', value: 'weekday' }
];

/**
 * Case-insensitive label sort. A plain comparator rather than localeCompare so
 * the ordering is identical on every phone JS engine.
 *
 * @param {Object} a First option.
 * @param {Object} b Second option.
 * @returns {number} Sort order.
 */
function byLabel(a, b) {
    var left = a.label.toLowerCase();
    var right = b.label.toLowerCase();

    if (left < right) {
        return -1;
    }

    return left > right ? 1 : 0;
}

/* "Empty" is not a metric, so it stays pinned at the end instead of sorting in
   between Distance and Heart rate. */
var SLOT_OPTIONS = METRIC_OPTIONS.slice().sort(byLabel).concat([
    { label: 'Empty', value: 'none' }
]);

/**
 * Decode the capability record the watch reported.
 *
 * Caps from a different watch are discarded: someone moving between a Pebble 2
 * SE and a Time 2 must not be offered the previous watch's metrics.
 *
 * @param {string|null} raw JSON string cached in localStorage.
 * @param {Object|null} watchInfo Result of Pebble.getActiveWatchInfo().
 * @returns {{caps: number, slots: number}|null} Usable caps, or null when unknown.
 */
function parseCaps(raw, watchInfo) {
    var record;

    if (!raw) {
        return null;
    }

    try {
        record = JSON.parse(raw);
    }
    catch (ex) {
        return null;
    }

    if (!record || typeof record.caps !== 'number') {
        return null;
    }

    var platform = watchInfo ? watchInfo.platform : null;
    if (platform && record.platform && record.platform !== platform) {
        return null;
    }

    return { caps: record.caps, slots: record.slots };
}

/**
 * Whether the watch reported it can supply a metric.
 *
 * @param {number} caps Capability bitmask from the watch.
 * @param {string} name Metric name from statsMetrics.METRIC_IDS.
 * @returns {boolean} True when the metric is supported.
 */
function supports(caps, name) {
    var id = statsMetrics.METRIC_IDS[name];

    // 'none' is always selectable: it is how a slot is left blank.
    if (id === statsMetrics.METRIC_IDS.none) {
        return true;
    }

    return Boolean(caps & (1 << id));
}

/**
 * Restrict each slot dropdown to metrics the connected watch can supply.
 *
 * Runs against clay.config before generateUrl() serializes it, because Clay's
 * select renders its options once from a template and exposes no runtime API to
 * change them.
 *
 * @param {Array} config Clay config array (clay.config, already deep-copied).
 * @param {string|null} rawCaps Cached capability JSON.
 * @param {Object|null} watchInfo Result of Pebble.getActiveWatchInfo().
 * @param {Object|null} settings Stored Clay settings; these, not the item
 *     defaults, are what the page will display.
 * @returns {void}
 */
function filterSlotOptions(config, rawCaps, watchInfo, settings) {
    var parsed = parseCaps(rawCaps, watchInfo);
    var stored = settings || {};

    eachSlotItem(config, function(item) {
        // With no usable report (fresh install, or caps from another watch)
        // offer everything: clay.config lives for the whole PKJS session, so an
        // earlier watch's filtered list must not linger.
        if (!parsed) {
            item.options = SLOT_OPTIONS;
            return;
        }

        var allowed = SLOT_OPTIONS.filter(function(option) {
            return supports(parsed.caps, option.value);
        });

        // Never drop the stored value: Clay would render a select whose value is
        // absent from its own options and silently lose the setting on save.
        // Clay shows the stored setting when there is one, else the default.
        var current = Object.prototype.hasOwnProperty.call(stored, item.messageKey)
            ? stored[item.messageKey]
            : item.defaultValue;
        var hasCurrent = allowed.some(function(option) {
            return option.value === current;
        });
        if (!hasCurrent) {
            allowed = allowed.concat(SLOT_OPTIONS.filter(function(option) {
                return option.value === current;
            }));
        }

        item.options = allowed;
    });
}

/**
 * Visit every statSlotN item in a Clay config, descending into sections.
 *
 * @param {Array|Object} node Clay config array or item.
 * @param {Function} callback Receives the slot item.
 * @returns {void}
 */
function eachSlotItem(node, callback) {
    if (Array.isArray(node)) {
        node.forEach(function(child) {
            eachSlotItem(child, callback);
        });
        return;
    }

    if (!node || typeof node !== 'object') {
        return;
    }

    if (Array.isArray(node.items)) {
        eachSlotItem(node.items, callback);
        return;
    }

    if (/^statSlot[1-9]$/.test(node.messageKey || '')) {
        callback(node);
    }
}

module.exports = {
    SLOT_OPTIONS: SLOT_OPTIONS,
    filterSlotOptions: filterSlotOptions
};
