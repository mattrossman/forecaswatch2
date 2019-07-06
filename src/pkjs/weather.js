function request(url, type, callback) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function () {
        callback(this.responseText);
    };
    xhr.open(type, url);
    xhr.send();
}

class WeatherProvider {
    constructor() {
        this.numEntries = 12;
    }

    withCityName(lat, lon, callback) {
        // callback(cityName)
        var url = 'https://nominatim.openstreetmap.org/reverse?lat=' + lat
                + '&lon=' + lon
                + '&format=json';
        request(url, 'GET',  function(response) {
            var location = JSON.parse(response);
            console.log('Running callback with city: ' + location.address.city);
            callback(location.address.city);
        });
    }

    withCoordinates(callback) {
        // callback(lattitude, longtitude)
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

    withProviderData(lat, lon, callback) {
        console.log('This is the fallback implementation of withProviderData')
        callback();
    }

    fetch(callback) {
        this.withCoordinates((function(lat, lon) {
            this.withCityName(lat, lon, (function(cityName) {
                this.withProviderData(lat, lon, (function() {
                    // if this. contains valid weather details:
                    // payload = this.getPayload()
                    if (this.hasValidData()) {
                        console.log('Lets get the payload for ' + cityName);
                        console.log('Forecast start time: ' + this.startHour);
                    }
                    else {
                        console.log('Fetch cancelled.')
                    }
                }).bind(this));
            }).bind(this));
        }).bind(this));
    }

    hasValidData() {
        // all fields are set
        if (this.tempTrend && this.precipTrend && this.startHour) {
            // trends are filled with enough data
            if (this.tempTrend.length >= this.numEntries && this.precipTrend.length >= this.numEntries) {
                console.log('Data is good, ready to fetch.');
                return true;
            }
        }
        console.log('Data is does not pass the checks.');
        return false;
    }
    
    getPayload() {
        var head = darkskyReponse.hourly.data.slice(0, config.numEntries);
        // Get the rounded (integer) temperatures for those hours
        var temps = this.tempTrend.slice(0, this.numEntries).map(function(temperature) {
            return Math.round(temperature);
        });
        var precips = this.precipTrend.slice(0, this.numEntries).map(function(probability) {
            return Math.round(probility * 100);
        });
        var tempsIntView = new Int16Array(temps);
        var tempsByteArray = Array.prototype.slice.call(new Uint8Array(tempsIntView.buffer))
        var payload = {
            'TEMP_TREND_INT16': tempsByteArray,
            'PRECIP_TREND_UINT8': precips, // Holds values within [0,100]
            'TEMP_START': this.startHour,
            'NUM_ENTRIES': config.numEntries,
            'CURRENT_TEMP': currentTemp,
            'CITY': location.address.city
        }
        return payload;
    }
}

class DarkSkyProvider extends WeatherProvider {
    constructor(apiKey) {
        super();
        this.apiKey = apiKey;
    }

    withDarkSkyResponse(lat, lon, callback) {
        // callback(darkSkyResponse)
        var url = 'https://api.darksky.net/forecast/' + this.apiKey + '/' + lat + ',' + lon + '?exclude=minutely,daily,alerts,flags';
        request(url, 'GET', function (response) {
            var weatherData = JSON.parse(response);
            console.log('Found timezone: ' + weatherData.timezone);
            callback(weatherData);
        });
    }

    withProviderData(lat, lon, callback) {
        // callBack expects that this.hasValidData() will be true
        console.log('This is the overriden implementation of withProviderData')
        this.tempTrend = [2, 2, 2, 4, 7, 9, 11, 12, 12, 12, 11, 9];
        this.precipTrend = [2, 2, 2, 4, 7, 9, 11, 12, 12, 12, 11, 9];
        this.startHour = 6;
        this.withDarkSkyResponse(lat, lon, (function(darkSkyResponse) {
            this.tempTrend = darkskyReponse.hourly.data.map(function(entry) {
                return entry.temperature;
            })
            this.precipTrend = darkskyReponse.hourly.data.map(function(entry) {
                return entry.precipProbability;
            })
            this.startHour = new Date(darkskyReponse.hourly.data[0].time * 1000).getHours()
            callback();
        }).bind(this));
    }
}

module.exports.DarkSkyProvider = DarkSkyProvider;