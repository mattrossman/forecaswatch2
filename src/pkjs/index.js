var config = require('./config.js');
// var DarkSkyProvider = require('./weather/darksky.js');
var WundergroundProvider = require('./weather/wunderground.js');
var Clay = require('./clay/_source.js');
var clayConfig = require('./clay/config.json');
var customClay = require('./clay/inject.js');
var clay = new Clay(clayConfig, customClay, { autoHandleEvents: false });

// var provider = new DarkSkyProvider(config.darkSkyApiKey);
var provider = new WundergroundProvider(config.wundergroundApiKey);

Pebble.addEventListener('showConfiguration', function(e) {
    Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
    if (e && !e.response) {
        return;
    }

    var dict = clay.getSettings(e.response, false);
    console.log(dict.keyProvider.value);
});

// Listen for when the watchface is opened
Pebble.addEventListener('ready',
    function (e) {
        console.log('PebbleKit JS ready!');
        tryFetch();
    }
);

function fetch() {
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

function tryFetch() {
    if (needRefresh()) {
        fetch();
    };
}

setInterval(function() {
    console.log('Tick from PKJS!');
    tryFetch();
}, 60 * 1000); // 60 * 1000 milsec = 1 minute

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