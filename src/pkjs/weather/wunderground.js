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
    var _this = this;
    this._super.call(this);
    this.name = 'Weather Underground';
    this.id = 'wunderground';
    var wundergroundApiKey = localStorage.getItem('wundergroundApiKey');
    if (wundergroundApiKey === null) {
        WundergroundProvider.withApiKey(function(apiKey) {
            localStorage.setItem('wundergroundApiKey', apiKey);
            console.log("Extracted Weather Underground API key: " + apiKey);
            _this.apiKey = apiKey;
        });
    }
    else {
        console.log("Using saved API key for Weather Underground");
        _this.apiKey = wundergroundApiKey;
    }
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

WundergroundProvider.withApiKey = function(callback) {
    var url = "https://www.wunderground.com/";
    request(url, 'GET', function (response) {
        callback(response.match(/apiKey=([a-z0-9]*)/)[1]);
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