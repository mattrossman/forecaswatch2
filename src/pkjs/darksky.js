var provider = require('./provider.js');

function request(url, type, callback) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function() {
        callback(this.responseText);
    };
    xhr.open(type, url);
    xhr.send();
}

var DarkSkyProvider = function(apiKey) {
    this._super.call(this);
    this.apiKey = apiKey;
}

DarkSkyProvider.prototype = Object.create(provider.constructor.prototype);
DarkSkyProvider.prototype.constructor = DarkSkyProvider;
DarkSkyProvider.prototype._super = provider.constructor;

DarkSkyProvider.prototype.withDarkSkyResponse = function(lat, lon, callback) {
    // callback(darkSkyResponse)
    var url = 'https://api.darksky.net/forecast/' + this.apiKey + '/' + lat + ',' + lon + '?exclude=minutely,daily,alerts,flags';
    request(url, 'GET', function (response) {
        var weatherData = JSON.parse(response);
        console.log('Found timezone: ' + weatherData.timezone);
        callback(weatherData);
    });
}

DarkSkyProvider.prototype.withProviderData = function(lat, lon, callback) {
    // callBack expects that this.hasValidData() will be true
    console.log('This is the overriden implementation of withProviderData')
    this.tempTrend = [2, 2, 2, 4, 7, 9, 11, 12, 12, 12, 11, 9];
    this.precipTrend = [2, 2, 2, 4, 7, 9, 11, 12, 12, 12, 11, 9];
    this.startHour = 6;
    this.withDarkSkyResponse(lat, lon, (function(darkSkyResponse) {
        this.tempTrend = darkSkyResponse.hourly.data.map(function(entry) {
            return entry.temperature;
        })
        this.precipTrend = darkSkyResponse.hourly.data.map(function(entry) {
            return entry.precipProbability;
        })
        this.startHour = new Date(darkSkyResponse.hourly.data[0].time * 1000).getHours()
        callback();
    }).bind(this));
}

module.exports.constructor = DarkSkyProvider;