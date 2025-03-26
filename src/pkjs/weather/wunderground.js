var WeatherProvider = require('./provider.js');

function request(url, type, callback) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function() {
        callback(this.responseText);
    };
    xhr.open(type, url);
    xhr.send();
}

var WundergroundProvider = function() {
    this._super.call(this);
    this.name = 'Weather Underground';
    this.id = 'wunderground';
}

WundergroundProvider.prototype = Object.create(WeatherProvider.prototype);
WundergroundProvider.prototype.constructor = WundergroundProvider;
WundergroundProvider.prototype._super = WeatherProvider;

WundergroundProvider.prototype.withWundergroundForecast = function(lat, lon, apiKey, callback) {
    // callback(wundergroundResponse)
    var url = 'https://api.weather.com/v1/geocode/' + lat + '/' + lon + '/forecast/hourly/48hour.json?apiKey=' + apiKey + '&language=en-US';

    console.log("Requesting " + url)

    request(url, 'GET', function (response) {
        try {
            var weatherData = JSON.parse(response);
            callback(weatherData.forecasts);
        } catch (e) {
            console.error("Failed to parse Weather Underground forecast response, clearing potentially stale API key");
            this.clearApiKey()
            throw e
        }
    });
}

WundergroundProvider.prototype.withWundergroundCurrent = function(lat, lon, apiKey, callback) {
    // callback(wundergroundResponse)
    var url = 'https://api.weather.com/v3/wx/observations/current?language=en-US&units=e&format=json'
        + '&apiKey=' + apiKey
        + '&geocode=' + lat + ',' + lon;

    console.log("Requesting " + url)

    request(url, 'GET', (function (response) {
        try {
            var weatherData = JSON.parse(response);
            callback(weatherData.temperature);
        } catch (e) {
            console.error("Failed to parse Weather Underground current weather response, clearing potentially stale API key");
            this.clearApiKey()
            throw e
        }
    }).bind(this));
}

WundergroundProvider.prototype.clearApiKey = function() {
    localStorage.removeItem('wundergroundApiKey');
    console.log("Cleared API key");
}

WundergroundProvider.prototype.withApiKey = function(callback) {
    // callback(apiKey)

    var apiKey = localStorage.getItem('wundergroundApiKey');

    if (apiKey === null) {
        console.log("Fetching Weather Underground API key");

        var url = "https://www.wunderground.com/";

        request(url, 'GET', function (response) {
            apiKey = response.match(/observations\/current\?apiKey=([a-z0-9]*)/)[1]
            localStorage.setItem('wundergroundApiKey', apiKey);
            console.log("Fetched Weather Underground API key: " + apiKey);

            callback(apiKey);
        });
    } else {
        console.log("Using saved API key for Weather Underground");
        callback(apiKey)
    }
}

// ============== IMPORTANT OVERRIDE ================

WundergroundProvider.prototype.withProviderData = function(lat, lon, force, callback) {
    // callback expects that this.hasValidData() will be true

    if (force) {
        // In case the API key becomes invalid
        console.log("Clearing Weather Underground API key for forced update")
        this.clearApiKey()
    }

    this.withApiKey((function(apiKey) {
        this.withWundergroundCurrent(lat, lon, apiKey, (function(currentTemp) {
            this.withWundergroundForecast(lat, lon, apiKey, (function(forecast) {
                this.tempTrend = forecast.map(function(entry) {
                    return entry.temp;
                })
                this.precipTrend = forecast.map(function(entry) {
                    return entry.pop / 100.0
                })
                this.startTime = forecast[0].fcst_valid;
                this.currentTemp = currentTemp;
                callback();
            }).bind(this));
        }).bind(this))
    }).bind(this));
}

module.exports = WundergroundProvider;