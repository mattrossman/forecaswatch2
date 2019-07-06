var config = require('./config.js');
var DarkSkyProvider = require('./weather/darksky.js');

var provider = new DarkSkyProvider(config.apiKey);

// Listen for when the watchface is opened
Pebble.addEventListener('ready',
    function (e) {
        console.log('PebbleKit JS ready!');
        tryFetch();
    }
);

function tryFetch() {
    ifDataIsOld(function() {
        provider.fetch(function() {
            // Sucess, update recent fetch time
            window.localStorage.setItem('fetchTime', new Date());
            console.log('Successfully fetched weather!')
        },
        function() {
            // Failure
            console.log('[!] Provider failed to update weather')
        })
    });
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

function ifDataIsOld(callback) {
    if (!window.localStorage.getItem('fetchTime')) {
        console.log('fetchTime not found!');
        callback();
    }
    else {
        lastFetchTime = new Date(window.localStorage.getItem('fetchTime'))
        if (Date.now() - roundDownMinutes(lastFetchTime, 30) > 1000 * 60 * 30) { // 1000 ms -> 60 sec -> 30 min
            console.log('Last fetch is is too old!');
            callback();
        }
    }
}