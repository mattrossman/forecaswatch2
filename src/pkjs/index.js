var config = require('./config.js');
// var DarkSkyProvider = require('./weather/darksky.js');
var WundergroundProvider = require('./weather/wunderground.js');
var Clay = require('./clay/_source.js');
var clayConfig = require('./clay/config.json');
var customClay = require('./clay/inject.js');
var clay = new Clay(clayConfig, customClay, { autoHandleEvents: false });

Pebble.addEventListener('showConfiguration', function(e) {
    Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
    if (e && !e.response) {
        return;
    }

    var settings = clay.getSettings(e.response, false);
    console.log(settings.provider.value);
});

// Listen for when the watchface is opened
Pebble.addEventListener('ready',
    function (e) {
        console.log('PebbleKit JS ready!');
        var provider = initProvider();
        startTick(provider);
    }
);

function startTick(provider) {
    console.log('Tick from PKJS!');
    tryFetch(provider);
    setTimeout(startTick, 60 * 1000); // 60 * 1000 milsec = 1 minute
}

function initProvider() {
    if (!localStorage.getItem('clay-settings')) {
        console.log('No Clay settings found, setting defaults');
        claySetDefaults();
    }
    var settings = JSON.parse(localStorage.getItem('clay-settings'));
    switch (settings.provider) {
        case 'wunderground':
            var provider = new WundergroundProvider(config.wundergroundApiKey);
            break;
        case 'darksky':
            var provider = new DarkSkyProvider(config.darkSkyApiKey);
            break;
    }
    console.log('Using provider: ' + settings.provider);
    return provider;
}

function claySetDefaults() {
    var settings = {
        provider: 'wunderground'
    }
    localStorage.setItem('clay-settings', JSON.stringify(settings));
}

function fetch(provider) {
    provider.fetch(function() {
        // Sucess, update recent fetch time
        window.localStorage.setItem('fetchTime', new Date());
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
    // Has the weather ever been fetched?
    if (!window.localStorage.getItem('fetchTime')) {
        return true;
    }
    // Is the most recent fetch more than 30 minutes old?
    lastFetchTime = new Date(window.localStorage.getItem('fetchTime'))
    return (Date.now() - roundDownMinutes(lastFetchTime, 30) > 1000 * 60 * 30);
}