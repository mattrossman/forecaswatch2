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
    this.weatherDataCache = null;
    console.log('Constructed with ' + apiKey);
}

OpenWeatherMapProvider.prototype = Object.create(WeatherProvider.prototype);
OpenWeatherMapProvider.prototype.constructor = OpenWeatherMapProvider;
OpenWeatherMapProvider.prototype._super = WeatherProvider;

OpenWeatherMapProvider.prototype.withOwmResponse = function (lat, lon, callback) {
    var url = 'https://api.openweathermap.org/data/3.0/onecall?appid=' + this.apiKey + '&lat=' + lat + '&lon=' + lon + '&units=imperial&exclude=alerts,minutely';

    console.log("Requesting " + url)
    
    request(url, 'GET', function (response) {
        var weatherData = JSON.parse(response);
        console.log('Found timezone: ' + weatherData.timezone);
        // cache weather data (use same request for sun events and weather forecast)
        this.weatherDataCache = weatherData;
        callback(weatherData);
    });
}

OpenWeatherMapProvider.prototype.withWeatherData = function (lat, lon, callback) {
    if (this.weatherDataCache === null) {
        this.withOwmResponse(lat, lon, function (owmResponse) {
            callback(owmResponse);
        });
    } else {
        callback(this.weatherDataCache);
    }
}

// ============== IMPORTANT OVERRIDE ================
OpenWeatherMapProvider.prototype.withSunEvents = function (lat, lon, callback) {
    console.log('This is the overridden implementation of withSunEvents')
    this.withOwmResponse(lat, lon, (function (owmResponse) {
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
        console.log('The next ' + sunEvents[0].type + ' is at ' + sunEvents[0].date.toTimeString());
        console.log('The next ' + sunEvents[1].type + ' is at ' + sunEvents[1].date.toTimeString());
        callback(next24HourSunEvents);
    }).bind(this));
}

OpenWeatherMapProvider.prototype.withProviderData = function (lat, lon, force, callback) {
    // callBack expects that this.hasValidData() will be true
    console.log('This is the overridden implementation of withProviderData')
    this.withWeatherData(lat, lon, (function (weatherData) {
        this.tempTrend = weatherData.hourly.map(function (entry) {
            return entry.temp;
        })
        this.precipTrend = weatherData.hourly.map(function (entry) {
            return entry.pop;
        })
        this.daysTemp = weatherData.daily.map(function (entry) {
            return entry.temp.max;
        })
        this.daysPop = weatherData.daily.map(function (entry) {
            return entry.pop;
        })
        this.daysIcon = weatherData.daily.map(function (entry) {
            return entry.weather[0].id;
        })     
        this.startTime = weatherData.hourly[0].dt;
        this.currentTemp = weatherData.current.temp;
        callback();
    }).bind(this))
}

module.exports = OpenWeatherMapProvider;