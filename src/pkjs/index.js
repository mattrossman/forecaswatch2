
var WundergroundProvider = require('./weather/wunderground.js');
var OpenWeatherMapProvider = require('./weather/openweathermap.js')
var MockProvider = require('./weather/mock.js');
var createTelemetryClient = require('./telemetry.js');
var Clay = require('./clay/_source.js');
var clayConfig = require('./clay/config.js');
var customClay = require('./clay/inject.js');
var pkg = require('../../package.json');

/**
 * Full release-notification manifest (dev: force-show by version). Omitted from bundle if missing.
 *
 * @returns {Object|null} Parsed release-notifications.json or null.
 */
function loadReleaseNotificationsManifest() {
    try {
        return require('../../release-notifications.json');
    }
    catch (ex) {
        return null;
    }
}

var releaseNotificationsManifest = loadReleaseNotificationsManifest();
var clay = new Clay(clayConfig, customClay, { autoHandleEvents: false });
var app = {};  // Namespace for global app variables
var KEY_MAX_NOTIFIED_VERSION = 'max_notified_version';
var KEY_FETCH_ATTEMPT = 'weather_fetch_attempt';

Pebble.addEventListener('showConfiguration', function(e) {
    // Set the userData here rather than in the Clay() constructor so it's actually up to date
    clay.meta.userData.lastFetchSuccess = localStorage.getItem('lastFetchSuccess');
    Pebble.openURL(clay.generateUrl());
    console.log('Showing clay: ' + JSON.stringify(getClaySettings()));
});

Pebble.addEventListener('webviewclosed', function(e) {
    if (e && !e.response) {
        return;
    }

    clay.getSettings(e.response, false);  // This triggers the update in localStorage
    app.settings = getClaySettings();  // This reads from localStorage in sensible format
    app.telemetry = createTelemetryClient(getRuntimeTelemetryConfig());
    refreshProvider();
    sendClaySettings();

    // Fetching goes last, after other settings have been handled
    if (app.settings.fetch === true) {
        console.log('Force fetch!');
        fetch(app.provider, true);
    }
    console.log('Closing clay: ' + JSON.stringify(getClaySettings()));
});

// Listen for when the watchface is opened
Pebble.addEventListener('ready',
    function (e) {
        app.devConfig = getDevConfig();
        maybeHandleDevStorageReset(app.devConfig);
        var hadExistingInstall = localStorage.getItem('clay-settings') !== null;
        maybeShowReleaseNotification(
            hadExistingInstall,
            app.devConfig.forceShowReleaseNotificationOnBoot
        );
        clayTryDefaults();
        clayTryDevConfig(app.devConfig);
        console.log('PebbleKit JS ready!');
        app.settings = getClaySettings();
        try {
            app.watchInfo = Pebble.getActiveWatchInfo();
        }
        catch (ex) {
            app.watchInfo = null;
            console.log('Unable to read watch info: ' + ex.message);
        }
        app.telemetry = createTelemetryClient(getRuntimeTelemetryConfig());
        refreshProvider();
        startTick();
    }
);

/**
 * Build telemetry runtime config from package.json.
 *
 * @returns {{enabled: boolean, endpoint: string, appVersion: string, buildProfile: string}} Runtime telemetry config.
 */
function getRuntimeTelemetryConfig() {
    var telemetry = pkg.telemetry || {};
    var endpoint = typeof telemetry.endpoint === 'string' ? telemetry.endpoint : '';
    var telemetryEnabled = !app.settings || app.settings.telemetryEnabled !== false;

    return {
        enabled: telemetryEnabled,
        endpoint: endpoint,
        appVersion: pkg.version,
        buildProfile: pkg.buildProfile
    };
}

/**
 * Parse a semver-like string into numeric major/minor/patch parts.
 *
 * @param {string} v Version string such as "1.25.0" or "v1.25.0-beta+build".
 * @returns {number[]} Tuple-like array: [major, minor, patch].
 */
function parseSemver(v) {
    var core = String(v || '0.0.0').replace(/^v/, '').split('-')[0].split('+')[0];
    var p = core.split('.');
    return [
        parseInt(p[0], 10) || 0,
        parseInt(p[1], 10) || 0,
        parseInt(p[2], 10) || 0
    ];
}

/**
 * Compare two semver-like version strings.
 *
 * @param {string} a Left-hand version.
 * @param {string} b Right-hand version.
 * @returns {number} 1 when a>b, -1 when a<b, 0 when equal.
 */
function compareSemver(a, b) {
    var pa = parseSemver(a);
    var pb = parseSemver(b);
    if (pa[0] !== pb[0]) return pa[0] > pb[0] ? 1 : -1;
    if (pa[1] !== pb[1]) return pa[1] > pb[1] ? 1 : -1;
    if (pa[2] !== pb[2]) return pa[2] > pb[2] ? 1 : -1;
    return 0;
}

/**
 * Normalize bundled pkg.releaseNotification into title/body or null.
 *
 * @param {Object|undefined} releaseNotification Field from package.json.
 * @returns {{title: string, body: string}|null} Payload or null when disabled/empty.
 */
function getBundledReleaseNotificationPayload(releaseNotification) {
    if (
        !releaseNotification ||
        releaseNotification.enabled !== true
    ) {
        return null;
    }
    var title = releaseNotification.title ? String(releaseNotification.title).trim() : '';
    var body = releaseNotification.body ? String(releaseNotification.body).trim() : '';
    if (title === '' || body === '') {
        return null;
    }
    return { title: title, body: body };
}

/**
 * Look up a release notification in release-notifications.json (dev force-show).
 *
 * @param {string} versionKey Exact version key, e.g. "1.26.0".
 * @returns {{title: string, body: string}|null} Payload or null when missing/invalid.
 */
function getReleaseNotificationFromManifest(versionKey) {
    var manifest = releaseNotificationsManifest;
    if (!manifest || typeof manifest !== 'object' || Array.isArray(manifest)) {
        return null;
    }
    var entry = Object.prototype.hasOwnProperty.call(manifest, versionKey)
        ? manifest[versionKey]
        : undefined;
    if (!entry || typeof entry !== 'object' || Array.isArray(entry)) {
        return null;
    }
    var title = entry.title ? String(entry.title).trim() : '';
    var body = entry.body ? String(entry.body).trim() : '';
    if (title === '' || body === '') {
        return null;
    }
    return { title: title, body: body };
}

/**
 * Parse dev-config force-show value: non-empty string = manifest version key.
 *
 * @param {*} forceVersionSpec From dev-config.forceShowReleaseNotificationOnBoot.
 * @returns {string} Trimmed version key or '' when disabled.
 */
function normalizeForceReleaseVersionSpec(forceVersionSpec) {
    if (typeof forceVersionSpec !== 'string') {
        return '';
    }
    return forceVersionSpec.trim();
}

/**
 * Show the release notification exactly once for eligible upgrades, or every boot when dev forces a manifest version.
 *
 * @param {boolean} hadExistingInstall True when this launch is not first install.
 * @param {*} forceVersionSpec Dev: exact version key in release-notifications.json (e.g. "1.26.0"), or falsy.
 * @returns {void}
 */
function maybeShowReleaseNotification(hadExistingInstall, forceVersionSpec) {
    var appVersion = pkg.version;
    var forceKey = normalizeForceReleaseVersionSpec(forceVersionSpec);
    var forcePayload = forceKey !== '' ? getReleaseNotificationFromManifest(forceKey) : null;
    if (forceKey !== '' && !forcePayload) {
        console.log(
            '[release-notification] force version ' + JSON.stringify(forceKey) +
            ' not found or invalid in release-notifications.json'
        );
    }

    var bundledPayload = getBundledReleaseNotificationPayload(pkg.releaseNotification);
    var maxNotified = localStorage.getItem(KEY_MAX_NOTIFIED_VERSION) || '0.0.0';
    var isNewer = compareSemver(appVersion, maxNotified) > 0;
    var shouldNotifyUpgrade = hadExistingInstall && isNewer && bundledPayload !== null;
    var shouldNotifyForce = forcePayload !== null;
    var shouldNotify = shouldNotifyUpgrade || shouldNotifyForce;
    var title = '';
    var body = '';
    if (shouldNotifyForce) {
        title = forcePayload.title;
        body = forcePayload.body;
    }
    else if (shouldNotifyUpgrade) {
        title = bundledPayload.title;
        body = bundledPayload.body;
    }

    console.log(
        '[release-notification] appVersion=' + appVersion +
        ' hadExistingInstall=' + hadExistingInstall +
        ' maxNotified=' + maxNotified +
        ' isNewer=' + isNewer +
        ' forceVersionKey=' + (forceKey !== '' ? forceKey : '(none)') +
        ' shouldNotify=' + shouldNotify +
        ' shouldNotifyUpgrade=' + shouldNotifyUpgrade +
        ' shouldNotifyForce=' + shouldNotifyForce +
        ' bundledPayload=' + Boolean(bundledPayload)
    );

    if (!shouldNotify) {
        console.log('[release-notification] skip');
    }

    if (shouldNotify) {
        console.log('[release-notification] showing notification');
        Pebble.showSimpleNotificationOnPebble(title, body);
    }

    if (isNewer) {
        localStorage.setItem(KEY_MAX_NOTIFIED_VERSION, appVersion);
        console.log('[release-notification] set max_notified_version=' + appVersion);
    }
    else {
        console.log('[release-notification] keep max_notified_version=' + maxNotified);
    }
}

/**
 * Optionally clear PKJS localStorage on boot when enabled in dev-config.js.
 *
 * @param {Object} devConfig Developer configuration object.
 * @returns {void}
 */
function maybeHandleDevStorageReset(devConfig) {
    var shouldClear = !!(devConfig && devConfig.clearPkjsStorageOnBoot);

    if (shouldClear) {
        console.log('[dev] clearPkjsStorageOnBoot=true, clearing localStorage');
        localStorage.clear();
    }
}

/**
 * Read the persisted weather fetch attempt counter.
 *
 * @returns {number} Non-negative integer attempt counter.
 */
function getFetchAttemptCounter() {
    var raw = localStorage.getItem(KEY_FETCH_ATTEMPT);
    var parsed = Number(raw);

    if (!isFinite(parsed) || parsed < 0) {
        return 0;
    }

    return Math.floor(parsed);
}

/**
 * Increment and persist the weather fetch attempt counter.
 *
 * @returns {number} New attempt number after increment.
 */
function incrementFetchAttemptCounter() {
    var nextAttempt = getFetchAttemptCounter() + 1;
    localStorage.setItem(KEY_FETCH_ATTEMPT, String(nextAttempt));
    return nextAttempt;
}

/**
 * Reset the weather fetch attempt counter after success.
 *
 * @returns {void}
 */
function resetFetchAttemptCounter() {
    localStorage.setItem(KEY_FETCH_ATTEMPT, '0');
}

function startTick() {
    console.log('Tick from PKJS!');
    tryFetch(app.provider);
    setTimeout(startTick, 60 * 1000); // 60 * 1000 milsec = 1 minute
}

function sendClaySettings() {
    var payload = {
        "CLAY_CELSIUS": app.settings.temperatureUnits === 'c',
        "CLAY_TIME_LEAD_ZERO": app.settings.timeLeadingZero,
        "CLAY_AXIS_12H": app.settings.axisTimeFormat === '12h',
        "CLAY_COLOR_TODAY": app.settings.hasOwnProperty('colorToday') ? app.settings.colorToday : 16777215,
        "CLAY_START_MON": app.settings.weekStartDay === 'mon',
        "CLAY_PREV_WEEK": app.settings.firstWeek === 'prev',
        "CLAY_TIME_FONT": ['roboto', 'leco', 'bitham'].indexOf(app.settings.timeFont),
        "CLAY_SHOW_QT": app.settings.showQt,
        "CLAY_SHOW_BT": app.settings.btIcons === "connected" || app.settings.btIcons === "both",
        "CLAY_SHOW_BT_DISCONNECT": app.settings.btIcons === "disconnected" || app.settings.btIcons === "both",
        "CLAY_VIBE": app.settings.vibe,
        "CLAY_SHOW_AM_PM": app.settings.timeShowAmPm,
        "CLAY_COLOR_SUNDAY": app.settings.hasOwnProperty('colorSunday') ? app.settings.colorSunday : 16777215,
        "CLAY_COLOR_SATURDAY": app.settings.hasOwnProperty('colorSaturday') ? app.settings.colorSaturday : 16777215,
        "CLAY_COLOR_US_FEDERAL": app.settings.hasOwnProperty('colorUSFederal') ? app.settings.colorUSFederal : 16777215,
        "CLAY_COLOR_TIME": app.settings.hasOwnProperty('colorTime') ? app.settings.colorTime : 16777215,
        "CLAY_DAY_NIGHT_SHADING": app.settings.hasOwnProperty('dayNightShading') ? app.settings.dayNightShading : true,
    }
    Pebble.sendAppMessage(payload, function() {
        console.log('Message sent successfully: ' + JSON.stringify(payload));
    }, function(e) {
        console.log('Message failed: ' + JSON.stringify(e));
    });
}

function refreshProvider() {
    setProvider(app.settings.provider);
    app.provider.location = app.settings.location === '' ? null : app.settings.location
}

function setProvider(providerId) {
    switch (providerId) {
        case 'openweathermap':
            app.provider = new OpenWeatherMapProvider(app.settings.owmApiKey);
            break;
        case 'wunderground':
            app.provider = new WundergroundProvider();
            break;
        case 'mock':
            app.provider = new MockProvider(app.devConfig || {});
            break;
        default:
            console.log('Unknown provider: "' + providerId + '", defaulting to wunderground');
            clay.setSettings("provider", "wunderground");
            app.provider = new WundergroundProvider();
    }
    console.log('Set provider: ' + app.provider.name);
}

function clayTryDefaults() {
    /* Clay only considers `defaultValue` upon first startup, but we need
     * defaults set even if the user has not made a custom config
     */
    var persistClayString = localStorage.getItem('clay-settings');
    var persistClay;
    if (persistClayString === null) {
        console.log('No clay settings found, setting defaults');
        persistClay = {
            provider: 'wunderground',
            location: '',
            dayNightShading: true,
            telemetryEnabled: true,
        }
        localStorage.setItem('clay-settings', JSON.stringify(persistClay));
        return;
    }

    try {
        persistClay = JSON.parse(persistClayString);
    }
    catch (ex) {
        console.log('Malformed clay settings found, resetting defaults');
        persistClay = {
            provider: 'wunderground',
            location: '',
            dayNightShading: true,
            telemetryEnabled: true,
        }
        localStorage.setItem('clay-settings', JSON.stringify(persistClay));
        return;
    }

    if (!Object.prototype.hasOwnProperty.call(persistClay, 'telemetryEnabled')) {
        persistClay.telemetryEnabled = true;
        localStorage.setItem('clay-settings', JSON.stringify(persistClay));
    }

}

function getDevConfig() {
    try {
        return require('./dev-config.js');
    }
    catch (ex) {
        console.log('No developer configuration file found');
        return {};
    }
}

function clayTryDevConfig(devConfig) {
    /* Use values from a dev-config.js file to configure clay settings
     * by iterating over the exported properties
     */
    var persistClay;
    var prop;

    var localOnlyDevConfigKeys = {
        emuTime: true,
        emuTimeFormat: true,
        mockCity: true,
        mockScenario: true,
        clearPkjsStorageOnBoot: true,
        forceShowReleaseNotificationOnBoot: true,
    };

    persistClay = getClaySettings();
    for (prop in devConfig) {
        if (Object.prototype.hasOwnProperty.call(devConfig, prop)) {
            if (Object.prototype.hasOwnProperty.call(localOnlyDevConfigKeys, prop)) {
                console.log('Found local-only dev setting: ' + prop);
                continue;
            }
            persistClay[prop] = devConfig[prop];
            console.log('Found dev setting: ' + prop + '=' + devConfig[prop]);
        }
    }
    localStorage.setItem('clay-settings', JSON.stringify(persistClay));
}

function getClaySettings() {
    return JSON.parse(localStorage.getItem('clay-settings'));
}

/**
 * Determine whether a watch is currently connected.
 *
 * @returns {boolean} True when a watch is connected.
 */
function isWatchConnected() {
    try {
        return Boolean(Pebble.getActiveWatchInfo());
    }
    catch (ex) {
        console.log('Unable to read active watch info: ' + ex.message);
        return false;
    }
}

/**
 * @typedef {import("./weather/provider")} WeatherProvider
 * @param {WeatherProvider} provider 
 * @param {boolean} force 
 */
function fetch(provider, force) {
    if (!isWatchConnected()) {
        console.log('Skipping weather fetch: no watch connected.');
        return;
    }

    console.log('Fetching from ' + provider.name);
    var fetchStart = Date.now();
    var attempt = incrementFetchAttemptCounter();
    var fetchStatus = {
        time: new Date(),
        id: provider.id,
        name: provider.name
    }
    localStorage.setItem('lastFetchAttempt', JSON.stringify(fetchStatus));
    provider.fetch(
        function() {
            // Sucess, update recent fetch time
            localStorage.setItem('lastFetchSuccess', JSON.stringify(fetchStatus));
            resetFetchAttemptCounter();
            console.log('Successfully fetched weather!');
            maybeTrackWeatherFetch({
                provider: provider.id,
                success: true,
                attempt: attempt,
                usedGpsCache: provider.usedGpsCache,
                gpsErrorCode: provider.gpsErrorCode,
                countryCode: provider.countryCode,
                settings: app.settings,
                watchInfo: app.watchInfo,
                durationMs: Date.now() - fetchStart
            });
        },
        function(failure) {
            // Failure
            console.log('[!] Provider failed to update weather: ' + JSON.stringify(failure));
            maybeTrackWeatherFetch({
                provider: provider.id,
                success: false,
                attempt: attempt,
                usedGpsCache: provider.usedGpsCache,
                gpsErrorCode: provider.gpsErrorCode,
                countryCode: provider.countryCode,
                error: failure,
                settings: app.settings,
                watchInfo: app.watchInfo,
                durationMs: Date.now() - fetchStart
            });
        },
        force
    )
}

/**
 * Send a weather fetch telemetry event when telemetry is enabled.
 *
 * @param {Object} event Telemetry event details.
 * @returns {void}
 */
function maybeTrackWeatherFetch(event) {
    if (!app.telemetry || app.telemetry.enabled !== true) {
        return;
    }
    app.telemetry.trackWeatherFetch(event || {});
}

function tryFetch(provider) {
    if (needRefresh()) {
        fetch(provider, false);
    };
}

function roundDownMinutes(date, minuteMod) {
    // E.g. with minuteMod=30, 3:52 would roll back to 3:30
    out = new Date(date);
    out.setMinutes(date.getMinutes() - (date.getMinutes() % minuteMod));
    out.setSeconds(0);
    out.setMilliseconds(0);
    return out;
}

function needRefresh() {
    // If the weather has never been fetched
    var lastFetchSuccessString = localStorage.getItem('lastFetchSuccess');
    if (lastFetchSuccessString === null) {
        return true;
    }
    var lastFetchSuccess = JSON.parse(lastFetchSuccessString);
    if (lastFetchSuccess.time === null) {
        // Just covering all my bases
        return true;
    }
    // If the most recent fetch is more than 30 minutes old
    return (Date.now() - roundDownMinutes(new Date(lastFetchSuccess.time), 30) > 1000 * 60 * 30);
}
