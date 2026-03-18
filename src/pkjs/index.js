
var WundergroundProvider = require('./weather/wunderground.js');
var OpenWeatherMapProvider = require('./weather/openweathermap.js')
var MockProvider = require('./weather/mock.js');
var createTelemetryClient = require('./telemetry.js');
var Clay = require('./clay/_source.js');
var clayConfig = require('./clay/config.js');
var customClay = require('./clay/inject.js');
var pkg = require('../../package.json');
var clay = new Clay(clayConfig, customClay, { autoHandleEvents: false });
var app = {};  // Namespace for global app variables
var KEY_MAX_NOTIFIED_VERSION = 'max_notified_version';

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
            !!app.devConfig.forceShowReleaseNotificationOnBoot
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
 * @returns {{endpoint: string, appVersion: string, buildProfile: string}} Runtime telemetry config.
 */
function getRuntimeTelemetryConfig() {
    var telemetry = pkg.telemetry || {};
    var endpoint = typeof telemetry.endpoint === 'string' ? telemetry.endpoint : '';

    return {
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
 * Show the release notification exactly once for eligible upgrades.
 *
 * @param {boolean} hadExistingInstall True when this launch is not first install.
 * @param {boolean} forceShow True to show release notification on every boot in dev.
 * @returns {void}
 */
function maybeShowReleaseNotification(hadExistingInstall, forceShow) {
    var appVersion = pkg.version;
    var releaseNotification = pkg.releaseNotification;
    var releaseTitle = releaseNotification && releaseNotification.title ? String(releaseNotification.title).trim() : '';
    var releaseBody = releaseNotification && releaseNotification.body ? String(releaseNotification.body).trim() : '';
    var isNotificationEnabled = !!(
        releaseNotification &&
        releaseNotification.enabled === true
    );
    var hasReleaseTitle = releaseTitle !== '';
    var hasReleaseBody = releaseBody !== '';
    var hasReleaseNotification = !!(
        isNotificationEnabled &&
        hasReleaseTitle &&
        hasReleaseBody
    );
    var maxNotified = localStorage.getItem(KEY_MAX_NOTIFIED_VERSION) || '0.0.0';
    var isNewer = compareSemver(appVersion, maxNotified) > 0;
    var shouldNotify = (
        (hadExistingInstall && isNewer && hasReleaseNotification) ||
        (forceShow && hasReleaseNotification)
    );

    console.log(
        '[release-notification] appVersion=' + appVersion +
        ' hadExistingInstall=' + hadExistingInstall +
        ' maxNotified=' + maxNotified +
        ' isNewer=' + isNewer +
        ' forceShow=' + !!forceShow +
        ' shouldNotify=' + shouldNotify +
        ' releaseNotificationEnabled=' + isNotificationEnabled +
        ' hasReleaseTitle=' + hasReleaseTitle +
        ' hasReleaseBody=' + hasReleaseBody +
        ' hasReleaseNotification=' + hasReleaseNotification
    );

    if (!shouldNotify) {
        console.log('[release-notification] skip');
    }

    if (shouldNotify) {
        console.log('[release-notification] showing notification');
        Pebble.showSimpleNotificationOnPebble(releaseTitle, releaseBody);
    }

    if (isNewer) {
        localStorage.setItem(KEY_MAX_NOTIFIED_VERSION, appVersion);
        console.log('[release-notification] set max_notified_version=' + appVersion);
    } else {
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
        }
        localStorage.setItem('clay-settings', JSON.stringify(persistClay));
        return;
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
 * @typedef {import("./weather/provider")} WeatherProvider
 * @param {WeatherProvider} provider 
 * @param {boolean} force 
 */
function fetch(provider, force) {
    console.log('Fetching from ' + provider.name);
    var fetchStart = Date.now();
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
            console.log('Successfully fetched weather!');
            maybeTrackWeatherFetch({
                provider: provider.id,
                success: true,
                countryCode: provider.countryCode,
                settings: app.settings,
                watchInfo: app.watchInfo,
                durationMs: Date.now() - fetchStart
            });
        },
        function(failure) {
            // Failure
            console.log('[!] Provider failed to update weather');
            maybeTrackWeatherFetch({
                provider: provider.id,
                success: false,
                countryCode: provider.countryCode,
                errorStage: failure && failure.stage ? failure.stage : 'unknown',
                errorCode: failure && failure.code ? failure.code : 'unknown',
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
