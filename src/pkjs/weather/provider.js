function request(url, type, callback) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function() {
        callback(this.responseText);
    };
    xhr.open(type, url);
    xhr.send();
}

var WeatherProvider = function() {
    this.numEntries = 24;
    this.name = 'Template';
    this.id = 'interface';
}

WeatherProvider.prototype.withCityName = function(lat, lon, callback) {
    // callback(cityName)
    var url = 'https://nominatim.openstreetmap.org/reverse?lat=' + lat
        + '&lon=' + lon
        + '&format=json';
    request(url, 'GET', function (response) {
        var address = JSON.parse(response).address;
        var name = address.city != null ? address.city : address.town
        console.log('Running callback with city: ' + name);
        callback(name);
    });
}

WeatherProvider.prototype.withCoordinates = function(callback) {
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

WeatherProvider.prototype.withProviderData = function(lat, lon, callback) {
    console.log('This is the fallback implementation of withProviderData')
    callback();
}

WeatherProvider.prototype.fetch = function(onSuccess, onFailure) {
    this.withCoordinates((function(lat, lon) {
        this.withCityName(lat, lon, (function(cityName) {
            this.withProviderData(lat, lon, (function() {
                // if `this` (the provider) contains valid weather details,
                // then we can safely call this.getPayload()
                if (this.hasValidData()) {
                    console.log('Lets get the payload for ' + cityName);
                    console.log('Forecast start time: ' + this.startHour);
                    // Send to Pebble
                    this.cityName = cityName;
                    payload = this.getPayload();
                    Pebble.sendAppMessage(payload,
                        function (e) {
                            console.log('Weather info sent to Pebble successfully!');
                            onSuccess();
                        },
                        function (e) {
                            console.log('Error sending weather info to Pebble!');
                            onFailure();
                        }
                    );
                }
                else {
                    console.log('Fetch cancelled: insufficient data.')
                    onFailure();
                }
            }).bind(this));
        }).bind(this));
    }).bind(this));
}

WeatherProvider.prototype.hasValidData = function() {
    // all fields are set
    if (this.hasOwnProperty('tempTrend') && this.hasOwnProperty('precipTrend') && this.hasOwnProperty('startHour') && this.hasOwnProperty('currentTemp')) {
        // trends are filled with enough data
        if (this.tempTrend.length >= this.numEntries && this.precipTrend.length >= this.numEntries) {
            console.log('Data from ' + this.name + ' is good, ready to fetch.');
            return true;
        }
    }
    else {
        if (!this.hasOwnProperty('tempTrend')) {
            console.log('Temperature trend array was not set properly');
        }
        if (!this.hasOwnProperty('precipTrend')) {
            console.log('Precipitation trend array was not set properly');
        }
        if (!this.hasOwnProperty('startHour')) {
            console.log('Start hour value was not set properly');
        }
        if (!this.hasOwnProperty('currentTemp')) {
            console.log('Current temperature value was not set properly');
        }
        console.log('Data does not pass the checks.');
        return false;
    }
}

WeatherProvider.prototype.getPayload = function() {
    // Get the rounded (integer) temperatures for those hours
    var temps = this.tempTrend.slice(0, this.numEntries).map(function(temperature) {
        return Math.round(temperature);
    });
    var precips = this.precipTrend.slice(0, this.numEntries).map(function(probability) {
        return Math.round(probability * 100);
    });
    var tempsIntView = new Int16Array(temps);
    var tempsByteArray = Array.prototype.slice.call(new Uint8Array(tempsIntView.buffer))
    var payload = {
        'TEMP_TREND_INT16': tempsByteArray,
        'PRECIP_TREND_UINT8': precips, // Holds values within [0,100]
        'TEMP_START': this.startHour,
        'NUM_ENTRIES': this.numEntries,
        'CURRENT_TEMP': Math.round(this.currentTemp),
        'CITY': this.cityName
    }
    return payload;
}

module.exports = WeatherProvider;