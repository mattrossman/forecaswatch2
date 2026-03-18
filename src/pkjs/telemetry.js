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
 * Build telemetry-safe watch metadata.
 *
 * @param {Object} watchInfo Pebble active watch info.
 * @returns {{watchPlatform: string|null, watchModel: string|null}} Watch metadata.
 */
function buildWatchMetadata(watchInfo) {
    if (!watchInfo || typeof watchInfo !== 'object') {
        return {
            watchPlatform: null,
            watchModel: null
        };
    }

    return {
        watchPlatform: typeof watchInfo.platform === 'string' ? watchInfo.platform : null,
        watchModel: typeof watchInfo.model === 'string' ? watchInfo.model : null
    };
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
    var endpoint = options && typeof options.endpoint === 'string' ? options.endpoint.trim() : '';
    var appVersion = options && typeof options.appVersion === 'string' ? options.appVersion : '0.0.0';
    var buildProfile = options && typeof options.buildProfile === 'string' ? options.buildProfile : 'unknown';

    if (endpoint === '') {
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
        var watchMeta;

        if (endpoint === '') {
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

        watchMeta = buildWatchMetadata(event.watchInfo);

        send({
            eventType: 'weather_fetch',
            timestampUtc: new Date().toISOString(),
            accountToken: accountToken,
            watchToken: watchToken,
            provider: event.provider,
            success: !!event.success,
            errorStage: typeof event.errorStage === 'string' ? event.errorStage : null,
            errorCode: typeof event.errorCode === 'string' ? event.errorCode : null,
            countryCode: normalizeCountryCode(event.countryCode),
            settings: buildSettingsSnapshot(event.settings),
            appVersion: appVersion,
            buildProfile: buildProfile,
            watchPlatform: watchMeta.watchPlatform,
            watchModel: watchMeta.watchModel
        });
    }

    return {
        enabled: endpoint !== '',
        trackWeatherFetch: trackWeatherFetch
    };
}

module.exports = createTelemetryClient;
