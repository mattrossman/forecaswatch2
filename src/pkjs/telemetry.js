/**
 * Build a compact, allowlisted settings snapshot for telemetry.
 *
 * @param {Object} settings Clay settings object.
 * @returns {Object} Telemetry-safe settings snapshot.
 */
function buildSettingsSnapshot(settings) {
    var safe = settings || {};
    return {
        temperatureUnits: safe.temperatureUnits,
        dayNightShading: !!safe.dayNightShading,
        provider: safe.provider,
        axisTimeFormat: safe.axisTimeFormat,
        timeFont: safe.timeFont,
        timeLeadingZero: !!safe.timeLeadingZero,
        timeShowAmPm: !!safe.timeShowAmPm,
        weekStartDay: safe.weekStartDay,
        firstWeek: safe.firstWeek,
        showQt: !!safe.showQt,
        vibe: !!safe.vibe,
        btIcons: safe.btIcons
    };
}

/**
 * Normalize a country code to uppercase ISO-like format.
 *
 * @param {string|null|undefined} code Raw country code.
 * @returns {string|null} Normalized country code or null.
 */
function normalizeCountryCode(code) {
    if (typeof code !== 'string') {
        return null;
    }
    var trimmed = code.trim().toUpperCase();
    if (!/^[A-Z]{2,3}$/.test(trimmed)) {
        return null;
    }
    return trimmed;
}

/**
 * Normalize location mode to an allowlisted telemetry value.
 *
 * @param {string|null|undefined} mode Raw location mode.
 * @returns {string|null} Normalized location mode or null.
 */
function normalizeLocationMode(mode) {
    if (mode === 'gps' || mode === 'manual_coordinates' || mode === 'manual_address') {
        return mode;
    }
    return null;
}

/**
 * Build telemetry-safe WatchInfo snapshot.
 *
 * @param {Object} watchInfo Pebble active watch info.
 * @returns {Object} Normalized WatchInfo payload.
 */
function buildWatchInfoSnapshot(watchInfo) {
    var firmware = {};

    if (!watchInfo || typeof watchInfo !== 'object') {
        return {};
    }

    if (watchInfo.firmware && typeof watchInfo.firmware === 'object') {
        if (typeof watchInfo.firmware.major === 'number' && isFinite(watchInfo.firmware.major)) {
            firmware.major = Math.floor(watchInfo.firmware.major);
        }
        if (typeof watchInfo.firmware.minor === 'number' && isFinite(watchInfo.firmware.minor)) {
            firmware.minor = Math.floor(watchInfo.firmware.minor);
        }
        if (typeof watchInfo.firmware.patch === 'number' && isFinite(watchInfo.firmware.patch)) {
            firmware.patch = Math.floor(watchInfo.firmware.patch);
        }
        if (typeof watchInfo.firmware.suffix === 'string') {
            firmware.suffix = watchInfo.firmware.suffix;
        }
    }

    return {
        platform: typeof watchInfo.platform === 'string' ? watchInfo.platform : null,
        model: typeof watchInfo.model === 'string' ? watchInfo.model : null,
        language: typeof watchInfo.language === 'string' ? watchInfo.language : null,
        firmware: firmware
    };
}

/**
 * Truncate a string to a maximum length.
 *
 * @param {string} value Input string.
 * @param {number} maxLength Max number of characters.
 * @returns {string} Truncated string.
 */
function truncateString(value, maxLength) {
    if (typeof value !== 'string') {
        return '';
    }

    if (typeof maxLength !== 'number' || maxLength < 1) {
        return value;
    }

    if (value.length <= maxLength) {
        return value;
    }

    if (maxLength <= 3) {
        return value.slice(0, maxLength);
    }

    return value.slice(0, maxLength - 3) + '...';
}

/**
 * Normalize any failure value into a readable telemetry error string.
 *
 * @param {*} value Failure payload from fetch flow.
 * @param {number} maxLength Max serialized error length.
 * @returns {string} Human-readable error string.
 */
function serializeError(value, maxLength) {
    var out = '';
    var base;
    var name;
    var message;
    var stack;
    var lines;
    var frames;
    var i;

    if (value instanceof Error || (value && typeof value === 'object' && (typeof value.message === 'string' || typeof value.stack === 'string'))) {
        name = (typeof value.name === 'string' && value.name.trim() !== '') ? value.name.trim() : 'Error';
        message = typeof value.message === 'string' ? value.message.trim() : '';
        base = message !== '' ? (name + ': ' + message) : name;

        stack = typeof value.stack === 'string' ? value.stack : '';
        if (stack.trim() !== '') {
            lines = stack.split('\n').map(function(line) {
                return line.trim();
            }).filter(function(line) {
                return line !== '';
            });
            frames = [];
            for (i = 0; i < lines.length; i += 1) {
                if (lines[i] === base || lines[i] === message || lines[i].indexOf(name + ':') === 0) {
                    continue;
                }
                frames.push(lines[i]);
                if (frames.length >= 3) {
                    break;
                }
            }

            if (frames.length > 0) {
                out = base + ' | stack: ' + frames.join(' <- ');
            }
            else {
                out = base;
            }
        }
        else {
            out = base;
        }
    }
    else if (typeof value === 'string') {
        out = value.trim();
    }
    else if (value && typeof value === 'object') {
        if (typeof value.stage === 'string' && value.stage.trim() !== '' && typeof value.code === 'string' && value.code.trim() !== '') {
            out = value.stage.trim() + ': ' + value.code.trim();
            if (typeof value.detail === 'string' && value.detail.trim() !== '') {
                out += ' (' + value.detail.trim() + ')';
            }
        }
        else if (typeof value.message === 'string' && value.message.trim() !== '') {
            out = value.message.trim();
        }
        else {
            try {
                out = JSON.stringify(value);
            }
            catch (ex) {
                out = String(value);
            }
        }
    }
    else if (typeof value !== 'undefined' && value !== null) {
        out = String(value);
    }

    if (out === '') {
        out = 'unknown error';
    }

    return truncateString(out, maxLength);
}

/**
 * Create a telemetry client for weather fetch events.
 *
 * @param {Object} options Telemetry client options.
 * @param {string} options.endpoint Telemetry ingest endpoint.
 * @param {string} options.appVersion App version string.
 * @param {string} options.buildProfile Build profile string.
 * @returns {{enabled: boolean, trackWeatherFetch: Function}} Telemetry client.
 */
function createTelemetryClient(options) {
    var enabled = !options || options.enabled !== false;
    var endpoint = options && typeof options.endpoint === 'string' ? options.endpoint.trim() : '';
    var appVersion = options && typeof options.appVersion === 'string' ? options.appVersion : '0.0.0';
    var buildProfile = options && typeof options.buildProfile === 'string' ? options.buildProfile : 'unknown';

    if (!enabled) {
        console.log('[telemetry] disabled by user setting');
    }
    else if (endpoint === '') {
        console.log('[telemetry] disabled (no endpoint configured)');
    }
    else {
        console.log('[telemetry] enabled endpoint=' + endpoint);
    }

    function send(payload) {
        var xhr = new XMLHttpRequest();
        xhr.open('POST', endpoint);
        xhr.setRequestHeader('Content-Type', 'application/json');
        console.log('[telemetry] sending event=' + payload.eventType + ' endpoint=' + endpoint);
        xhr.onload = function() {
            if (xhr.status >= 200 && xhr.status < 300) {
                console.log('[telemetry] sent event=' + payload.eventType + ' status=' + xhr.status);
                return;
            }
            console.log('[telemetry] non-2xx status=' + xhr.status + ' body=' + xhr.responseText);
        };
        xhr.onerror = function() {
            console.log('[telemetry] request error');
        };
        xhr.send(JSON.stringify(payload));
    }

    /**
     * Track one weather fetch attempt.
     *
     * @param {Object} event Telemetry event properties.
     * @returns {void}
     */
    function trackWeatherFetch(event) {
        var accountToken;
        var watchToken;
        var watchInfo;
        var success;
        var error;
        var attempt;

        if (!enabled || endpoint === '') {
            console.log('[telemetry] telemetry disabled');
            return;
        }

        try {
            accountToken = Pebble.getAccountToken();
        }
        catch (ex) {
            console.log('[telemetry] getAccountToken failed: ' + ex.message);
            return;
        }

        if (typeof accountToken !== 'string' || accountToken.trim() === '') {
            console.log('[telemetry] getAccountToken returned empty value');
            return;
        }

        try {
            watchToken = Pebble.getWatchToken();
        }
        catch (ex) {
            watchToken = null;
            console.log('[telemetry] getWatchToken failed: ' + ex.message);
        }

        if (typeof watchToken !== 'string' || watchToken.trim() === '') {
            watchToken = null;
        }

        watchInfo = buildWatchInfoSnapshot(event.watchInfo);
        success = Boolean(event.success);
        error = success ? null : serializeError(event.error, 512);
        attempt = (
            typeof event.attempt === 'number' &&
            isFinite(event.attempt) &&
            event.attempt >= 1
        ) ? Math.floor(event.attempt) : null;

        send({
            eventType: 'weather_fetch',
            timestampUtc: new Date().toISOString(),
            accountToken: accountToken,
            watchToken: watchToken,
            provider: event.provider,
            success: success,
            usedGpsCache: event.usedGpsCache,
            gpsErrorCode: typeof event.gpsErrorCode === 'number' ? event.gpsErrorCode : null,
            locationMode: normalizeLocationMode(event.locationMode),
            error: error,
            countryCode: normalizeCountryCode(event.countryCode),
            settings: buildSettingsSnapshot(event.settings),
            appVersion: appVersion,
            buildProfile: buildProfile,
            watchInfo: watchInfo,
            durationMs: typeof event.durationMs === 'number' ? event.durationMs : null,
            attempt: attempt
        });
    }

    return {
        enabled: enabled && endpoint !== '',
        trackWeatherFetch: trackWeatherFetch
    };
}

module.exports = createTelemetryClient;
