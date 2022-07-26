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
    this.location = null;  // Address query used for overriding the GPS
}

WeatherProvider.prototype.gpsEnable = function() {
    this.location = null;
}

WeatherProvider.prototype.gpsOverride = function(location) {
    this.location = location;
}

WeatherProvider.prototype.withSunEvents = function(lat, lon, callback) {
    /* The callback runs with an array of the next two sun events (i.e. 24 hours worth),
     * where each sun event contains a 'type' ('sunrise' or 'sunset') and a 'date' (of type Date)
     */
    var urlToday = 'http://api.sunrise-sunset.org/json?formatted=0'
        + '&lat=' + lat
        + '&lng=' + lon;
    var urlTomorrow = urlToday + '&date=tomorrow';
    var processResults = function(results) {
        return [
            {
                'type': 'sunrise',
                'date': new Date(results.sunrise)
            },
            {
                'type': 'sunset',
                'date': new Date(results.sunset)
            }
        ]
    }
    request(urlToday, 'GET', function (responseToday) {
        var resultsToday = JSON.parse(responseToday).results;
        request(urlTomorrow, 'GET', function (responseTomorrow) {
            var resultsTomorrow = JSON.parse(responseTomorrow).results;
            var sunEvents = processResults(resultsToday).concat(processResults(resultsTomorrow));
            var now = new Date();
            var nextSunEvents = sunEvents.filter(function (sunEvent) {
                return sunEvent.date > now;
            });
            var next24HourSunEvents = nextSunEvents.slice(0, 2);
            console.log('The next ' + sunEvents[0].type + ' is at ' + sunEvents[0].date.toTimeString());
            console.log('The next ' + sunEvents[1].type + ' is at ' + sunEvents[1].date.toTimeString());
            callback(next24HourSunEvents);
        });
    });
}

WeatherProvider.prototype.withCityName = function(lat, lon, callback) {
    // callback(cityName)
    var url = 'https://geocode.arcgis.com/arcgis/rest/services/World/GeocodeServer/reverseGeocode?f=json&langCode=EN&location='
        + lon + ',' + lat;
    request(url, 'GET', function (response) {
        var address = JSON.parse(response).address;
        var name = address.District ? address.District : address.City;
        console.log('Running callback with city: ' + name);
        callback(name);
    });
}

WeatherProvider.prototype.withGeocodeCoordinates = function(callback) {
    // callback(lattitude, longtitude)
    var url = 'https://us1.locationiq.com/v1/search.php?key=dd15eccc31178e'
        + '&q=' + this.location
        + '&format=json';
    request(url, 'GET', function (response) {
        var locations = JSON.parse(response);
        if (locations.length === 0) {
            console.log('[!] No geocoding results')
        }
        else {
            var closest = locations[0];
            console.log('Query ' + this.location + ' geocoded to ' + closest.lat + ', ' + closest.lon);
            callback(closest.lat, closest.lon);
        }
    });
}

WeatherProvider.prototype.withGpsCoordinates = function(callback) {
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

WeatherProvider.prototype.withCoordinates = function(callback) {
    if (this.location === null) {
        console.log('Using GPS')
        this.withGpsCoordinates(callback);
    }
    else {
        console.log('Using geocoded coordinates')
        this.withGeocodeCoordinates(callback);
    }
}

WeatherProvider.prototype.withProviderData = function(lat, lon, callback) {
    console.log('This is the fallback implementation of withProviderData')
    callback();
}

WeatherProvider.prototype.fetch = function(onSuccess, onFailure) {
    this.withCoordinates((function(lat, lon) {
        this.withCityName(lat, lon, (function(cityName) {
            this.withSunEvents(lat, lon, (function(sunEvents) {
                this.withProviderData(lat, lon, (function() {
                    // if `this` (the provider) contains valid weather details,
                    // then we can safely call this.getPayload()
                    if (this.hasValidData()) {
                        console.log('Lets get the payload for ' + cityName);
                        console.log('Forecast start time: ' + this.startHour);
                        // Send to Pebble
                        this.cityName = cityName;
                        this.sunEvents = sunEvents;
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
    }).bind(this));
}

WeatherProvider.prototype.hasValidData = function() {
    // all fields are set
    if (this.hasOwnProperty('tempTrend') && this.hasOwnProperty('precipTrend') && this.hasOwnProperty('startTime') && this.hasOwnProperty('currentTemp')) {
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
        if (!this.hasOwnProperty('startTime')) {
            console.log('Start time value was not set properly');
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
    var sunEventsIntView = new Int32Array(this.sunEvents.map(function(sunEvent) {
        return sunEvent.date.getTime() / 1000;  // Seconds since epoch
    }));
    var sunEventsByteArray = Array.prototype.slice.call(new Uint8Array(sunEventsIntView.buffer))
    var payload = {
        'TEMP_TREND_INT16': tempsByteArray,
        'PRECIP_TREND_UINT8': precips, // Holds values within [0,100]
        'FORECAST_START': this.startTime,
        'NUM_ENTRIES': this.numEntries,
        'CURRENT_TEMP': Math.round(this.currentTemp),
        'CITY': this.cityName,
        // The first byte determines whether the list of events starts on a sunrise (0) or sunset (1)
        'SUN_EVENTS': [this.sunEvents[0].type == 'sunrise' ? 0 : 1].concat(sunEventsByteArray)
    }
    return payload;
}

module.exports = WeatherProvider;
