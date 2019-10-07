
var DarkSkyProvider = require('./weather/darksky.js');
var WundergroundProvider = require('./weather/wunderground.js');
var Clay = require('./clay/_source.js');
var clayConfig = require('./clay/config.js');
var customClay = require('./clay/inject.js');
var clay = new Clay(clayConfig, customClay, { autoHandleEvents: false });
var app = {};  // Namespace for global app variables

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
        fetch(app.provider);
    }
    console.log('Closing clay: ' + JSON.stringify(getClaySettings()));
});

// Listen for when the watchface is opened
Pebble.addEventListener('ready',
    function (e) {
        clayTryDefaults();
        clayTryDevConfig();
        console.log('PebbleKit JS ready!');
        app.settings = getClaySettings();
        refreshProvider();
        startTick();
    }
);

function startTick() {
    console.log('Tick from PKJS!');
    tryFetch(app.provider);
    setTimeout(startTick, 60 * 1000); // 60 * 1000 milsec = 1 minute
}

function sendClaySettings() {
    payload = {
        "CLAY_CELSIUS": app.settings.temperatureUnits === 'c',
        "CLAY_TIME_LEAD_ZERO": app.settings.timeLeadingZero,
        "CLAY_AXIS_12H": app.settings.axisTimeFormat === '12h',
        "CLAY_COLOR_TODAY": app.settings.hasOwnProperty('colorToday') ? app.settings.colorToday : 16777215,
        "CLAY_START_MON": app.settings.weekStartDay === 'mon'
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
        case 'wunderground':
            app.provider = new WundergroundProvider();
            break;
        case 'darksky':
            app.provider = new DarkSkyProvider(app.settings.darkSkyApiKey);
            break;
        default:
            console.log('Error assigning provider in initProvider');
    }
    console.log('Set provider: ' + app.provider.name);
}

function clayTryDefaults() {
    /* Clay only considers `defaultValue` upon first startup, but we need
     * defaults set even if the user has not made a custom config
     */
    var persistClay = localStorage.getItem('clay-settings');
    if (persistClay === null) {
        console.log('No clay settings found, setting defaults');
        persistClay = {
            provider: 'wunderground',
            location: ''
        }
        localStorage.setItem('clay-settings', JSON.stringify(persistClay));
    }
}

function clayTryDevConfig() {
    /* Use values from a dev-config.js file to configure clay settings
     * by iterating over the exported properties
     */
    try {
        var devConfig = require('./dev-config.js');
        var persistClay = getClaySettings();
        for (var prop in devConfig) {
            if (Object.prototype.hasOwnProperty.call(devConfig, prop)) {
                persistClay[prop] = devConfig[prop];
                console.log('Found dev setting: ' + prop + '=' + devConfig[prop]);
            }
        }
        localStorage.setItem('clay-settings', JSON.stringify(persistClay));
    }
    catch (ex) {
        console.log("No developer configuration file found");
    }
}

function getClaySettings() {
    return JSON.parse(localStorage.getItem('clay-settings'));
}

function fetch(provider) {
    console.log('Fetching from ' + provider.name);
    var fetchStatus = {
        time: new Date(),
        id: provider.id,
        name: provider.name
    }
    localStorage.setItem('lastFetchAttempt', JSON.stringify(fetchStatus));
    provider.fetch(function() {
        // Sucess, update recent fetch time
        localStorage.setItem('lastFetchSuccess', JSON.stringify(fetchStatus));
        console.log('Successfully fetched weather!')
    },
    function() {
        // Failure
        console.log('[!] Provider failed to update weather')
    })
}

function tryFetch(provider) {
    if (needRefresh()) {
        fetch(provider);
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