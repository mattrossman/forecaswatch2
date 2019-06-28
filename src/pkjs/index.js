var config = require('./config.js');

// Listen for when the watchface is opened
Pebble.addEventListener('ready',
    function (e) {
        console.log('PebbleKit JS ready!');
        ifDataIsOld(function() {
            withCoordinates(getWeather);
        });
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
    ifDataIsOld(function() {
        withCoordinates(getWeather);
    });
}, 60 * 1000); // 60 * 1000 milsec = 1 minute

function withCoordinates(callback) {
    var options = {
        enableHighAccuracy: true,
        maximumAge: 10000,
        timeout: 10000
    };
    function success(pos) {
        console.log('FOUND LOCATION: lat= ' + pos.coords.latitude + ' lon= ' + pos.coords.longitude);
        callback(pos.coords.latitude, pos.coords.longitude);
    }
    function error(err) {
        console.log('location error (' + err.code + '): ' + err.message);
    }
    navigator.geolocation.getCurrentPosition(success, error, options);
}

function ifDataIsOld(callback) {
    if (!window.localStorage.getItem('fetchTime')) {
        console.log('fetchTime not found, fetching weather!');
        callback();
    }
    else {
        lastFetchTime = parseFloat(window.localStorage.getItem('fetchTime'), 10);
        if (Date.now() - lastFetchTime >= 1000 * 60 * 30) { // 1000 ms * 60 sec * 60 min = 1 hour
            console.log('Existing data is too old, refetching!');
            callback();
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

function getWeather(lat, lon) {
    var url = 'https://api.darksky.net/forecast/' + config.apiKey + '/' + lat + ',' + lon + '?exclude=minutely,daily,alerts,flags';
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

    var trendIntView = new Int16Array(temperatures)
    var trendByteArray = Array.prototype.slice.call(new Uint8Array(trendIntView.buffer))

    // Calculate the starting time (hour) for the forecast
    var tempStartHour = new Date(twelveHours[0].time * 1000).getHours()

    locUrl = 'https://nominatim.openstreetmap.org/reverse?lat=' + darkskyReponse.latitude
            + '&lon=' + darkskyReponse.longitude
            + '&format=json';
    request(locUrl, 'GET',  function(response) {
        var location = JSON.parse(response);
        console.log('Forecast was fetched for ' + location.address.city);

        // Assemble the message keys
        var payload = {
            'ARRAY': trendByteArray,
            'TEMP_START': tempStartHour,
            'CITY': location.address.city
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
    })
}