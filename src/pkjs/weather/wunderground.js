var WeatherProvider = require('./provider.js');

function request(url, type, callback) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function() {
        callback(this.responseText);
    };
    xhr.open(type, url);
    xhr.send();
}

var WundergroundProvider = function(apiKey) {
    this._super.call(this);
    this.name = 'Weather Underground';
    this.apiKey = apiKey;
}

WundergroundProvider.prototype = Object.create(WeatherProvider.prototype);
WundergroundProvider.prototype.constructor = WundergroundProvider;
WundergroundProvider.prototype._super = WeatherProvider;

WundergroundProvider.prototype.withWundergroundResponse = function(lat, lon, callback) {
    // callback(wundergroundResponse)
    var url = 'https://api.weather.com/v1/geocode/' + lat + '/' + lon + '/forecast/hourly/48hour.json?apiKey=' + this.apiKey;
    request(url, 'GET', function (response) {
        var weatherData = JSON.parse(response);
        callback(weatherData);
    });
}

// ============== IMPORTANT OVERRIDE ================

WundergroundProvider.prototype.withProviderData = function(lat, lon, callback) {
    // callBack expects that this.hasValidData() will be true
    this.withWundergroundResponse(lat, lon, (function(wundergroundResponse) {
        var forecast = wundergroundResponse.forecasts;
        this.tempTrend = forecast.map(function(entry) {
            return entry.temp;
        })
        this.precipTrend = forecast.map(function(entry) {
            return entry.pop / 100.0
        })
        this.startHour = new Date(forecast[0].fcst_valid * 1000).getHours()
        this.currentTemp = this.tempTrend[0];
        callback();
    }).bind(this));
}

module.exports = WundergroundProvider;