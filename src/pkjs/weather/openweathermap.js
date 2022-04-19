var WeatherProvider = require('./provider.js');

function request(url, type, callback) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function () {
        callback(this.responseText);
    };
    xhr.open(type, url);
    xhr.send();
}

var OpenWeatherMapProvider = function (apiKey) {
    this._super.call(this);
    this.name = 'OpenWeatherMap';
    this.id = 'openweathermap';
    this.apiKey = apiKey;
    this.weatherData = null;
    console.log('Constructed with ' + apiKey);
}

OpenWeatherMapProvider.prototype = Object.create(WeatherProvider.prototype);
OpenWeatherMapProvider.prototype.constructor = OpenWeatherMapProvider;
OpenWeatherMapProvider.prototype._super = WeatherProvider;

OpenWeatherMapProvider.prototype.withOwmResponse = function (lat, lon, callback) {
    var url = `https://api.openweathermap.org/data/2.5/onecall?appid=${this.apiKey}&lat=${lat}&lon=${lon}&exclude=alerts,minutely`;

    request(url, 'GET', function (response) {
        var weatherData = JSON.parse(response);
        console.log('Found timezone: ' + weatherData.timezone);
        callback(weatherData);
    });
}

// ============== IMPORTANT OVERRIDE ================
OpenWeatherMapProvider.prototype.withSunEvents = function (lat, lon, callback) {

    this.withOwmResponse(lat, lon, (function (owmResponse) {
        // cache the weather response to provide the rest of the data later
        this.weatherData = owmResponse;

        var days = owmResponse.daily;
        var sunEvents = [
            { 'type': 'sunrise', 'date': new Date(days[0].sunrise * 1000) },
            { 'type': 'sunset', 'date': new Date(days[0].sunset * 1000) },
            { 'type': 'sunrise', 'date': new Date(days[1].sunrise * 1000) },
            { 'type': 'sunset', 'date': new Date(days[1].sunset * 1000) }
        ]
        var now = new Date();
        var nextSunEvents = sunEvents.filter(function (sunEvent) {
            return sunEvent.date > now;
        });
        var next24HourSunEvents = nextSunEvents.slice(0, 2);
        callback(next24HourSunEvents);
    }).bind(this));
}

OpenWeatherMapProvider.prototype.withProviderData = function (lat, lon, callback) {
    if (this.weatherData !== null) {
        this.tempTrend = owmResponse.hourly.map(function (entry) {
            return entry.temp;
        })
        this.precipTrend = owmResponse.hourly.map(function (entry) {
            return entry.pop;
        })
        this.startTime = owmResponse.hourly[0].dt;
        this.currentTemp = owmResponse.current.temp;

        callback();
    }
}

module.exports = OpenWeatherMapProvider;