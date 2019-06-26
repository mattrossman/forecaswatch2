var config = require('./config.js');

// Listen for when the watchface is opened
Pebble.addEventListener('ready',
    function (e) {
        console.log('PebbleKit JS ready!');
        tryWeatherUpdate();
    }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
    function (e) {
        console.log('AppMessage received!');
        getWeather();
    }                     
);

setInterval(function() {
    console.log('Tick from PKJS!');
    tryWeatherUpdate();
}, 60 * 1000); // 60 * 1000 milsec

function tryWeatherUpdate() {
    if (!window.localStorage.getItem('fetchTime')) {
        console.log('fetchTime not found, fetching weather!');
        getWeather();
    }
    else {
        lastFetchTime = parseFloat(window.localStorage.getItem('fetchTime'), 10);
        if (Date.now() - lastFetchTime >= 1000 * 60 * 30) { // 1000 ms * 60 sec * 60 min = 1 hour
            console.log('Existing data is too old, refetching!');
            getWeather();
        }
    }
}

function request(url, type, callback) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function () {
        callback(this.responseText);
    };
    xhr.open(type, url);
    xhr.send();
}

function getWeather() {
    var url = 'https://api.darksky.net/forecast/' + config.apiKey + '/' + config.lat + ',' + config.lon + '?exclude=minutely,daily,alerts,flags';
    request(url, 'GET', function (response) {
        var weatherData = JSON.parse(response);
        console.log('Found timezone: ' + weatherData.timezone);
        processDarkskyResponse(weatherData);
        console.log('Setting fetchTime in local storage');
        window.localStorage.setItem('fetchTime', Date.now());
    });
}

function processDarkskyResponse(darkskyReponse) {
    // Get the first twelve hours of the hourly forecast
    twelveHours = darkskyReponse.hourly.data.slice(0,12);

    // Get the rounded (integer) temperatures for those hours
    temperatures = twelveHours.map(function(entry){
        return Math.round(entry.temperature);
    });

    // Extract the low and high temperatures
    lo = Math.min.apply(Math, temperatures);
    hi = Math.max.apply(Math, temperatures);
    console.log('Lo: ' + lo + ', Hi: ' + hi);

    var trendIntView = new Int16Array(temperatures)
    var trendByteArray = Array.prototype.slice.call(new Uint8Array(trendIntView.buffer))

    // Assemble the message keys
    var payload = {
        'TEMP_LO': lo,
        'TEMP_HI': hi,
        'ARRAY': trendByteArray
    }

    // Send to Pebble
    Pebble.sendAppMessage(payload,
        function (e) {
            console.log('Weather info sent to Pebble successfully!');
        },
        function (e) {
            console.log('Error sending weather info to Pebble!');
        }
    );
}