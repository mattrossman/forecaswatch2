const SunCalc = require('suncalc')
const RiDuck = require('../riduck/riduck')
const OpenHolidays = require('../openholidaysapi/openholidaysapi')

function request(url, type, callback) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function () {
        callback(this.responseText);
    };
    xhr.open(type, url);
    xhr.send();
}

var WeatherProvider = function () {
    this.numEntries = 24;
    this.numDays = 7;
    this.name = 'Template';
    this.id = 'interface';
    this.advice = 0;
    this.uvi = 0;
    this.holidays = 0;
    this.location = null;  // Address query used for overriding the GPS
    this.riduckUser = '';
    this.riduckPassword = '';
    this.openHolidaysCountry = '';
    this.openHolidaysRegional = '';
}

WeatherProvider.prototype.gpsEnable = function () {
    this.location = null;
}

WeatherProvider.prototype.gpsOverride = function (location) {
    this.location = location;
}

WeatherProvider.prototype.withSunEvents = function (lat, lon, callback) {
    /* The callback runs with an array of the next two sun events (i.e. 24 hours worth),
     * where each sun event contains a 'type' ('sunrise' or 'sunset') and a 'date' (of type Date)
     */
    const dateNow = new Date()
    const dateTomorrow = new Date().setDate(dateNow.getDate() + 1)

    const resultsToday = SunCalc.getTimes(dateNow, lat, lon)
    const resultsTomorrow = SunCalc.getTimes(dateTomorrow, lat, lon)

    /**
     * @param {SunCalc.GetTimesResult} results 
     * @returns {{ type: 'sunrise'|'sunset', date: Date }[]}
     */
    var processResults = function (results) {
        return [
            {
                'type': 'sunrise',
                'date': results.sunrise
            },
            {
                'type': 'sunset',
                'date': results.sunset
            }
        ]
    }

    var sunEvents = processResults(resultsToday).concat(processResults(resultsTomorrow))
    var nextSunEvents = sunEvents.filter(function (sunEvent) {
        return sunEvent.date > dateNow;
    });
    var next24HourSunEvents = nextSunEvents.slice(0, 2);
    console.log('The next ' + sunEvents[0].type + ' is at ' + sunEvents[0].date.toTimeString());
    console.log('The next ' + sunEvents[1].type + ' is at ' + sunEvents[1].date.toTimeString());
    callback(next24HourSunEvents);
}

WeatherProvider.prototype.withCityName = function (lat, lon, callback) {
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

// https://github.com/mattrossman/forecaswatch2/issues/59#issue-1317582743
const r_lat_long = new RegExp(/([-+]?[\d\.]*),([-+]?[\d\.]*)/gm);

WeatherProvider.prototype.withGeocodeCoordinates = function (callback) {
    // callback(lattitude, longtitude)
    var locationiq_key = 'pk.5a61972cde94491774bcfaa0705d5a0d';
    var url = 'https://us1.locationiq.com/v1/search.php?key=' + locationiq_key
        + '&q=' + this.location
        + '&format=json';
    var m = r_lat_long.exec(this.location);

    console.log('WeatherProvider.prototype.withGeocodeCoordinates lets regex, this.location: ' + JSON.stringify(this.location));
    if (m != null) {
        var latitude = m[1];
        var longitude = m[2];

        console.log('regex matched, override is lat/long')
        callback(latitude, longitude);
    }
    else {
        console.log('regex failed, about to look up lat/long for override')
        request(url, 'GET', function (response) {
            var locations = JSON.parse(response);
            if (locations.length === 0) {
                console.log('[!] No geocoding results')
            }
            else {
                var closest = locations[0];
                console.log('Query ' + this.location + ' geocoded to ' + closest.lat + ', ' + closest.lon);
                JSON.stringify('closest.lat ' + JSON.stringify(closest.lat));
                JSON.stringify('closest ' + JSON.stringify(closest));
                callback(closest.lat, closest.lon);
            }
        });
    }

}

WeatherProvider.prototype.withGpsCoordinates = function (callback) {
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

WeatherProvider.prototype.withRiDuck = function (callback) {
    console.log('Trying to connect to RiDuck');
    if (this.riduckPassword === '' || this.riduckUser === '') {
        console.log('Riduck: Error: No RiDuck credentials');
        callback(0);
    }
    else {
        var riduck = new RiDuck();
        riduck.login(this.riduckUser, this.riduckPassword, function (token) {
            if (token === '') {
                console.log('Riduck: Error: Did not get RiDuck JWT');
                callback(0);
            }
            else {
                riduck.fetchAdvice(token, function (advice) {
                    callback(advice);
                }.bind(this));
            }
        }.bind(this));
    }
}

WeatherProvider.prototype.withOpenHolidays = function (callback) {
    console.log('Trying to connect to OpenHolidays');
    if (this.openHolidaysCountry === '') {
        console.log('Holiday: Error: No OpenHolidays country code given');
        callback(0);
    }
    else {
        var openHolidays = new OpenHolidays();
        openHolidays.getHolidayBitmask(this.openHolidaysCountry, this.openHolidaysRegional, function (bitmask) {
            callback(bitmask);
        }.bind(this));
    }
}

WeatherProvider.prototype.withCoordinates = function (callback) {
    if (this.location === null) {
        console.log('Using GPS')
        this.withGpsCoordinates(callback);
    }
    else {
        console.log('Using geocoded coordinates')
        this.withGeocodeCoordinates(callback);
    }
}

WeatherProvider.prototype.withProviderData = function (lat, lon, force, callback) {
    console.log('This is the fallback implementation of withProviderData')
    callback();
}

WeatherProvider.prototype.fetch = function (onSuccess, onFailure, force) {
    this.withCoordinates((function (lat, lon) {
        this.withCityName(lat, lon, (function (cityName) {
            this.withRiDuck((function (advice) {
                this.withOpenHolidays((function (holidays) {
                    this.withSunEvents(lat, lon, (function (sunEvents) {
                        this.withProviderData(lat, lon, force, (function () {
                            // if `this` (the provider) contains valid weather details,
                            // then we can safely call this.getPayload()
                            if (this.hasValidData()) {
                                console.log('Lets get the payload for ' + cityName);
                                // Send to Pebble
                                this.cityName = cityName;
                                this.sunEvents = sunEvents;
                                this.advice = advice;
                                this.holidays = holidays;
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
                                console.log('Error: Fetch cancelled: insufficient data.')
                                onFailure();
                            }
                        }).bind(this));
                    }).bind(this));
                }).bind(this));
            }).bind(this));
        }).bind(this));
    }).bind(this));
}

WeatherProvider.prototype.hasValidData = function () {
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
            console.log('Error: Temperature trend array was not set properly');
        }
        if (!this.hasOwnProperty('precipTrend')) {
            console.log('Error: Precipitation trend array was not set properly');
        }
        if (!this.hasOwnProperty('startTime')) {
            console.log('Error: Start time value was not set properly');
        }
        if (!this.hasOwnProperty('currentTemp')) {
            console.log('Error: Current temperature value was not set properly');
        }
        console.log('Error: Data does not pass the checks.');
        return false;
    }
}

WeatherProvider.prototype.getPayload = function () {
    // Get the rounded (integer) temperatures for those hours
    var temps = this.tempTrend.slice(0, this.numEntries).map(function (temperature) {
        return Math.round(temperature);
    });
    console.log("Step 1");
    var windSpeeds =
        this.windSpeed.slice(0, this.numEntries).map(function (windspeed) {
            if (windspeed > 255) windspeed = 255;
            return Math.round(windspeed);
        }
        );
    console.log("Step 2");
    var daysTemp = this.daysTemp.slice(0, this.numDays).map(function (temperature) {
        return Math.round(temperature);
    });
    var daysIcons = this.daysIcon.slice(0, this.numDays).map(function (temperature) {
        return temperature;
    });
    const precips = this.precipTrend.slice(0, this.numEntries).map(p => Math.round(p * 15));
    console.log("Step 3");
    const precips_amount = this.precipMMH.slice(0, this.numEntries).map(a => {
        if (a > 45) return 15;
        return Math.ceil(a * 0.33);
    });

    var combined = [];
    console.log("Step 4");
    for (let i = 0; i < this.numEntries; i++) {
        const high = precips[i] & 0x0F;         // ensure only 4 bits
        const low = precips_amount[i] & 0x0F;   // ensure only 4 bits
        combined[i] = (high << 4) | low;        // pack into one byte
    }
    console.log("Step 5");
    var daysPrecips = this.daysPop.slice(0, this.numDays).map(function (probability) {
        return Math.round(probability * 100);
    });
    var tempsIntView = new Int16Array(temps);
    var daysTempIntView = new Int16Array(daysTemp);
    var daysIconsIntView = new Int16Array(daysIcons);
    var tempsByteArray = Array.prototype.slice.call(new Uint8Array(tempsIntView.buffer))
    var daysTempsByteArray = Array.prototype.slice.call(new Uint8Array(daysTempIntView.buffer))
    var daysIconByteArray = Array.prototype.slice.call(new Uint8Array(daysIconsIntView.buffer))
    var sunEventsIntView = new Int32Array(this.sunEvents.map(function (sunEvent) {
        return sunEvent.date.getTime() / 1000;  // Seconds since epoch
    }));
    var sunEventsByteArray = Array.prototype.slice.call(new Uint8Array(sunEventsIntView.buffer))
    console.log("Step 6");
    var payload = {
        'TEMP_TREND_INT16': tempsByteArray,
        'TEMP_DAYS_INT16': daysTempsByteArray,
        'ICON_DAYS_INT16': daysIconByteArray,
        'PRECIP_DAYS_UINT8': daysPrecips,
        'PRECIP_TREND_UINT8': combined,
        'WINDSPEED_TREND_UINT8': windSpeeds,
        'FORECAST_START': this.startTime,
        'ADVICE': this.advice,
        'HOLIDAYS': this.holidays,
        'NUM_ENTRIES': this.numEntries,
        'NUM_DAYS': this.numDays,
        'CURRENT_TEMP': Math.round(this.currentTemp),
        'UVI': Math.round(this.uvi * 100),
        'CITY': this.cityName,
        // The first byte determines whether the list of events starts on a sunrise (0) or sunset (1)
        'SUN_EVENTS': [this.sunEvents[0].type == 'sunrise' ? 0 : 1].concat(sunEventsByteArray)
    }
    console.log("Step 7");
    return payload;
}

module.exports = WeatherProvider;
