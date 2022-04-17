var WeatherProvider = require('./provider.js');

function request(url, type, callback) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function () {
        callback(this.responseText);
    };
    xhr.open(type, url);
    xhr.send();
}

var OpenWeatherMapProvider = function () {
    var _this = this;
    this._super.call(this);
    this.name = 'OpenWeatherMap';
    this.id = 'openweathermap';
}

OpenWeatherMapProvider.prototype = Object.create(WeatherProvider.prototype);
OpenWeatherMapProvider.prototype.constructor = OpenWeatherMapProvider;
OpenWeatherMapProvider.prototype._super = WeatherProvider;

OpenWeatherMapProvider.prototype.withWundergroundForecast = function (lat, lon, callback) {
    // callback(wundergroundResponse)
    var url = 'https://api.weather.com/v1/geocode/' + lat + '/' + lon + '/forecast/hourly/48hour.json?apiKey=' + this.apiKey;
    request(url, 'GET', function (response) {
        var weatherData = JSON.parse(response);
        callback(weatherData.forecasts);
    });
}

OpenWeatherMapProvider.prototype.withWundergroundCurrent = function (lat, lon, callback) {
    // callback(wundergroundResponse)
    var url = 'https://api.weather.com/v3/wx/observations/current?language=en-US&units=e&format=json'
        + '&apiKey=' + this.apiKey
        + '&geocode=' + lat + ',' + lon;
    request(url, 'GET', function (response) {
        var weatherData = JSON.parse(response);
        callback(weatherData.temperature);
    });
}

OpenWeatherMapProvider.withApiKey = function (callback) {
    var url = "https://www.wunderground.com/";
    request(url, 'GET', function (response) {
        callback(response.match(/apiKey=([a-z0-9]*)/)[1]);
    });
}

// ============== IMPORTANT OVERRIDE ================

OpenWeatherMapProvider.prototype.withProviderData = function (lat, lon, callback) {
    // callBack expects that this.hasValidData() will be true
    this.withWundergroundCurrent(lat, lon, (function (currentTemp) {
        this.withWundergroundForecast(lat, lon, (function (forecast) {
            this.tempTrend = forecast.map(function (entry) {
                return entry.temp;
            })
            this.precipTrend = forecast.map(function (entry) {
                return entry.pop / 100.0
            })
            this.startTime = forecast[0].fcst_valid;
            this.currentTemp = currentTemp;
            callback();
        }).bind(this));
    }).bind(this))
}

module.exports = OpenWeatherMapProvider;