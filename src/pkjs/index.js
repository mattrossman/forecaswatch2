var devConfig = require('./dev-config.js');
var DarkSkyProvider = require('./weather/darksky.js');
var WundergroundProvider = require('./weather/wunderground.js');
var Clay = require('./clay/_source.js');
var clayConfig = require('./clay/config.json');
var customClay = require('./clay/inject.js');
var clay = new Clay(clayConfig, customClay, { autoHandleEvents: false });

Pebble.addEventListener('showConfiguration', function(e) {
    // Set the userData here rather than in the Clay() constructor so it's actually up to date
    clay.meta.userData.lastFetchTime = localStorage.getItem('fetchTime');
    Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
    if (e && !e.response) {
        return;
    }

    var settings = clay.getSettings(e.response, false);
    if (settings.fetch.value === true) {
        console.log('Force fetch!');
    }
});

// Listen for when the watchface is opened
Pebble.addEventListener('ready',
    function (e) {
        clayTryDefaults();
        console.log('PebbleKit JS ready!');
        var provider = initProvider()
        startTick(provider);
    }
);

function startTick(provider) {
    console.log('Tick from PKJS!');
    tryFetch(provider);
    setTimeout(startTick, 60 * 1000); // 60 * 1000 milsec = 1 minute
}

function initProvider() {
    var settings = JSON.parse(localStorage.getItem('clay-settings'));
    var provider;
    console.log("Settings: " + JSON.stringify(settings));
    switch (settings.provider) {
        case 'wunderground':
            provider = new WundergroundProvider(devConfig.wundergroundApiKey);
            break;
        case 'darksky':
            provider = new DarkSkyProvider(devConfig.darkSkyApiKey);
            break;
    }
    console.log('Initialized provider: ' + provider.name);
    return provider;
}

function clayTryDefaults() {
    /* Clay only considers `defaultValue` upon first startup, but we need
     * defaults set even if the user has not made a custom config
     */
    var persistClay = localStorage.getItem('clay-settings');
    if (persistClay === null) {
        persistClay = {
            provider: 'wunderground'
        }
    }
    localStorage.setItem('clay-settings', JSON.stringify(persistClay));
}

function fetch(provider) {
    console.log('Fetching from ' + provider.name);
    provider.fetch(function() {
        // Sucess, update recent fetch time
        localStorage.setItem('fetchTime', new Date());
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
    if (localStorage.getItem('fetchTime') === null) {
        return true;
    }
    // If the most recent fetch is more than 30 minutes old
    lastFetchTime = new Date(localStorage.getItem('fetchTime'))
    return (Date.now() - roundDownMinutes(lastFetchTime, 30) > 1000 * 60 * 30);
}