var SunCalc = require('suncalc');
var storageKeys = require('../storage-keys.js');

var XHR_TIMEOUT_MS = 5000;
var GPS_CACHE_KEY = 'gpsCache';
var GPS_CACHE_MAX_AGE_MS = 24 * 60 * 60 * 1000;
var GEOCODE_CACHE_KEY = storageKeys.GEOCODE_CACHE_KEY;
var RATE_LIMIT_BACKOFF_KEY = storageKeys.GEOCODE_BACKOFF_KEY;

/**
 * Perform an HTTP request and return response text.
 *
 * @param {string} url Request URL.
 * @param {string} type HTTP method.
 * @param {Function} onSuccess Callback with response text.
 * @param {Function} onFailure Callback with error details.
 * @returns {void}
 */
function request(url, type, onSuccess, onFailure) {
    var xhr = new XMLHttpRequest();
    xhr.timeout = XHR_TIMEOUT_MS;
    xhr.onload = function() {
        if (xhr.status >= 200 && xhr.status < 300) {
            onSuccess(this.responseText);
            return;
        }
        onFailure({
            code: 'status_' + xhr.status,
            detail: 'http_status'
        });
    };
    xhr.onerror = function() {
        onFailure({
            code: 'network_error',
            detail: 'xhr_error'
        });
    };
    xhr.ontimeout = function() {
        onFailure({
            code: 'timeout',
            detail: 'xhr_timeout'
        });
    };
    xhr.open(type, url);
    xhr.send();
}

/**
 * Build a normalized fetch failure payload.
 *
 * @param {string} stage Failure stage identifier.
 * @param {string} code Failure code identifier.
 * @returns {{stage: string, code: string}} Normalized failure object.
 */
function failure(stage, code) {
    return {
        stage: stage,
        code: code
    };
}

/**
 * Parse stored JSON and clear invalid values.
 *
 * @param {string} key localStorage key.
 * @returns {*} Parsed value or null when missing/invalid.
 */
function readStoredJson(key) {
    var raw = localStorage.getItem(key);

    if (raw === null) {
        return null;
    }

    try {
        return JSON.parse(raw);
    }
    catch (ex) {
        localStorage.removeItem(key);
        return null;
    }
}

/**
 * Read the cached geocode result for the active location.
 *
 * @param {string} location Query string.
 * @returns {{query: string, lat: string, lon: string, time: number}|null}
 */
function readGeocodeCache(location) {
    var cachedGeocode = readStoredJson(GEOCODE_CACHE_KEY);

    if (cachedGeocode && cachedGeocode.query === location) {
        return cachedGeocode;
    }

    return null;
}

/**
 * Persist a successful geocode lookup.
 *
 * @param {string} location Query string.
 * @param {string} lat Latitude.
 * @param {string} lon Longitude.
 * @returns {void}
 */
function writeGeocodeCache(location, lat, lon) {
    localStorage.setItem(GEOCODE_CACHE_KEY, JSON.stringify({
        query: location,
        lat: lat,
        lon: lon,
        time: Date.now()
    }));
}

/**
 * Determine whether geocoding is still in backoff.
 *
 * @returns {boolean} True when geocoding should be skipped.
 */
function isGeocodeBackoffActive() {
    var backoffData = readStoredJson(RATE_LIMIT_BACKOFF_KEY);

    if (!backoffData) {
        return false;
    }

    if (Date.now() < (backoffData.until || 0)) {
        return true;
    }

    localStorage.removeItem(RATE_LIMIT_BACKOFF_KEY);
    return false;
}

/**
 * Record a LocationIQ 429 backoff window.
 *
 * @returns {number} Backoff duration in milliseconds.
 */
function writeGeocodeBackoff() {
    var currentBackoff = readStoredJson(RATE_LIMIT_BACKOFF_KEY);
    var attempts = currentBackoff && currentBackoff.attempts ? currentBackoff.attempts : 0;
    var backoffMs = attempts > 0
        ? Math.min(30000 * Math.pow(2, attempts), 1800000)
        : 60000;

    localStorage.setItem(RATE_LIMIT_BACKOFF_KEY, JSON.stringify({
        until: Date.now() + backoffMs,
        attempts: attempts + 1
    }));

    return backoffMs;
}

var WeatherProvider = function() {
    this.numEntries = 24;
    this.name = 'Template';
    this.id = 'interface';
    this.location = null; // Address query used for overriding the GPS
    this.countryCode = null;
    this.usedGpsCache = false;
    this.gpsErrorCode = null;
    this.locationMode = null;
};

WeatherProvider.prototype.gpsEnable = function() {
    this.location = null;
};

WeatherProvider.prototype.gpsOverride = function(location) {
    this.location = location;
};

/**
 * Determine whether the provider is currently rate-limited for geocoding.
 *
 * @returns {boolean} True when forward geocoding should be skipped.
 */
WeatherProvider.prototype.isGeocodeBackoffActive = function() {
    if (parseLocationOverride(this.location).type !== 'manual_address') {
        return false;
    }

    return isGeocodeBackoffActive();
};

WeatherProvider.prototype.withSunEvents = function(lat, lon, callback, onFailure) {
    /* The callback runs with an array of the next two sun events (i.e. 24 hours worth),
     * where each sun event contains a 'type' ('sunrise' or 'sunset') and a 'date' (of type Date)
     */
    var dateNow = new Date();
    var dateTomorrow = new Date().setDate(dateNow.getDate() + 1);

    var resultsToday;
    var resultsTomorrow;

    try {
        resultsToday = SunCalc.getTimes(dateNow, lat, lon);
        resultsTomorrow = SunCalc.getTimes(dateTomorrow, lat, lon);
    }
    catch (ex) {
        onFailure(failure('sun_events', 'calc_error'));
        return;
    }

    /**
     * @param {SunCalc.GetTimesResult} results
     * @returns {{ type: 'sunrise'|'sunset', date: Date }[]}
     */
    var processResults = function(results) {
        return [
            {
                type: 'sunrise',
                date: results.sunrise
            },
            {
                type: 'sunset',
                date: results.sunset
            }
        ];
    };

    var sunEvents = processResults(resultsToday).concat(processResults(resultsTomorrow));
    var nextSunEvents = sunEvents.filter(function(sunEvent) {
        return sunEvent.date > dateNow;
    });
    var next24HourSunEvents = nextSunEvents.slice(0, 2);
    console.log('The next ' + sunEvents[0].type + ' is at ' + sunEvents[0].date.toTimeString());
    console.log('The next ' + sunEvents[1].type + ' is at ' + sunEvents[1].date.toTimeString());
    callback(next24HourSunEvents);
};

WeatherProvider.prototype.withCityName = function(lat, lon, callback, onFailure) {
    // callback(cityName, countryCode)
    var url = 'https://geocode.arcgis.com/arcgis/rest/services/World/GeocodeServer/reverseGeocode?f=json&langCode=EN&location='
        + lon + ',' + lat;

    request(
        url,
        'GET',
        function(response) {
            var body;
            var address;
            var name;
            var countryCode;
            try {
                body = JSON.parse(response);
            }
            catch (ex) {
                onFailure(failure('reverse_geocode', 'parse_error'));
                return;
            }

            address = body.address || {};
            name = address.District || address.City || address.Region || 'Unknown';
            countryCode = address.CountryCode || null;
            console.log('Running callback with city: ' + name + ', countryCode=' + countryCode);
            callback(name, countryCode);
        },
        function(error) {
            console.log('[!] Reverse geocode failed: ' + JSON.stringify(error));
            onFailure(failure('reverse_geocode', error.code));
        }
    );
};

// https://github.com/mattrossman/forecaswatch2/issues/59#issue-1317582743
var r_lat_long = /^([-+]?\d*\.?\d+)\s*,\s*([-+]?\d*\.?\d+)$/;

/**
 * Build the GPS override state.
 *
 * @returns {{ type: 'gps', query: null, latitude: null, longitude: null }} GPS override state.
 */
function createGpsLocationOverrideState() {
    return {
        type: 'gps',
        query: null,
        latitude: null,
        longitude: null
    };
}

/**
 * Parse a location override into GPS, manual coordinates, or an address.
 *
 * @param {*} location Location override value.
 * @returns {{ type: 'gps'|'manual_coordinates'|'manual_address', query: string|null, latitude: string|null, longitude: string|null }} Parsed override state.
 */
function parseLocationOverride(location) {
    var trimmedLocation;
    var match;

    if (typeof location !== 'string') {
        return createGpsLocationOverrideState();
    }

    trimmedLocation = location.trim();
    if (trimmedLocation.length === 0) {
        return createGpsLocationOverrideState();
    }

    match = trimmedLocation.match(r_lat_long);
    if (match !== null) {
        return {
            type: 'manual_coordinates',
            query: trimmedLocation,
            latitude: match[1],
            longitude: match[2]
        };
    }

    return {
        type: 'manual_address',
        query: trimmedLocation,
        latitude: null,
        longitude: null
    };
}

WeatherProvider.prototype.withGeocodeCoordinates = function(callback, onFailure) {
    // callback(latitude, longitude)
    var locationiqKey = 'pk.5a61972cde94491774bcfaa0705d5a0d';
    var locationOverride = parseLocationOverride(this.location);
    var url;
    var latitude;
    var longitude;
    var cachedGeocode;
    var backoffMs;

    console.log('WeatherProvider.prototype.withGeocodeCoordinates override: ' + JSON.stringify(this.location));
    if (locationOverride.type === 'manual_coordinates') {
        latitude = locationOverride.latitude;
        longitude = locationOverride.longitude;
        this.locationMode = 'manual_coordinates';
        console.log('regex matched, override is lat/long');
        callback(latitude, longitude);
        return;
    }

    if (locationOverride.type !== 'manual_address') {
        onFailure(failure('forward_geocode', 'invalid_location'));
        return;
    }

    url = 'https://us1.locationiq.com/v1/search.php?key=' + locationiqKey
        + '&q=' + encodeURIComponent(locationOverride.query)
        + '&format=json';

    // Keep cached coordinates usable even while LocationIQ is in backoff.
    cachedGeocode = readGeocodeCache(locationOverride.query);
    if (cachedGeocode !== null) {
        console.log('Using cached geocode for: ' + locationOverride.query);
        this.locationMode = 'manual_address';
        callback(cachedGeocode.lat, cachedGeocode.lon);
        return;
    }

    // Check rate limit backoff: skip geocoding if we're still in cooldown from a 429
    if (isGeocodeBackoffActive()) {
        console.log('[!] Geocoding in backoff cooldown, skipping');
        onFailure(failure('forward_geocode', 'backoff'));
        return;
    }

    this.locationMode = 'manual_address';
    console.log('Looking up coordinates for address override');
    request(
        url,
        'GET',
        (function(response) {
            var locations;
            var closest;
            try {
                locations = JSON.parse(response);
            }
            catch (ex) {
                onFailure(failure('forward_geocode', 'parse_error'));
                return;
            }

            if (!Array.isArray(locations) || locations.length === 0) {
                console.log('[!] No geocoding results');
                onFailure(failure('forward_geocode', 'no_results'));
                return;
            }

            closest = locations[0];
            console.log('Query ' + locationOverride.query + ' geocoded to ' + closest.lat + ', ' + closest.lon);
            // Cache the successful geocode result
            writeGeocodeCache(locationOverride.query, closest.lat, closest.lon);
            callback(closest.lat, closest.lon);
        }).bind(this),
        (function(error) {
            console.log('[!] Forward geocode failed: ' + JSON.stringify(error));

            // Apply exponential backoff on 429 responses
            if (error.code === 'status_429') {
                backoffMs = writeGeocodeBackoff();
                console.log('[!] LocationIQ 429, backing off for ' + (backoffMs / 1000) + 's');
            }
            else {
                // Clear backoff on non-429 errors (e.g. network issues)
                localStorage.removeItem(RATE_LIMIT_BACKOFF_KEY);
            }
            onFailure(failure('forward_geocode', error.code));
        }).bind(this)
    );
};

WeatherProvider.prototype.withGpsCoordinates = function(callback, onFailure) {
    // callback(latitude, longitude)
    var provider = this;
    var options = {
        enableHighAccuracy: true,
        maximumAge: 10000,
        timeout: 10000
    };

    provider.usedGpsCache = false;
    provider.gpsErrorCode = null;

    function success(pos) {
        var lat = pos.coords.latitude;
        var lon = pos.coords.longitude;
        console.log('FOUND LOCATION: lat= ' + lat + ' lon= ' + lon);
        localStorage.setItem(GPS_CACHE_KEY, JSON.stringify({
            lat: lat,
            lon: lon,
            time: Date.now()
        }));
        provider.usedGpsCache = false;
        provider.gpsErrorCode = null;
        callback(lat, lon);
    }

    function error(err) {
        var cached;
        var parsed;
        var cacheIsFresh;
        var errCode;
        console.log('location error (' + err.code + '): ' + err.message);

        errCode = Number(err && err.code);
        provider.gpsErrorCode = errCode;

        cached = localStorage.getItem(GPS_CACHE_KEY);
        if (cached !== null) {
            try {
                parsed = JSON.parse(cached);
            }
            catch (ex) {
                parsed = null;
            }

            cacheIsFresh = true;
            if (GPS_CACHE_MAX_AGE_MS > 0) {
                cacheIsFresh = (
                    parsed &&
                    typeof parsed.time === 'number' &&
                    Date.now() - parsed.time <= GPS_CACHE_MAX_AGE_MS
                );
            }

            if (
                parsed &&
                typeof parsed.lat === 'number' &&
                typeof parsed.lon === 'number' &&
                cacheIsFresh
            ) {
                console.log('Using cached GPS coordinates: lat= ' + parsed.lat + ' lon= ' + parsed.lon);
                provider.usedGpsCache = true;
                provider.gpsErrorCode = errCode;
                callback(parsed.lat, parsed.lon);
                return;
            }
        }

        onFailure(failure('coordinates', 'gps_' + err.code));
    }

    navigator.geolocation.getCurrentPosition(success, error, options);
};

WeatherProvider.prototype.withCoordinates = function(callback, onFailure) {
    var locationOverride;

    this.usedGpsCache = false;
    this.gpsErrorCode = null;
    this.locationMode = null;

    locationOverride = parseLocationOverride(this.location);
    if (locationOverride.type === 'gps') {
        this.locationMode = 'gps';
        console.log('Using GPS');
        this.withGpsCoordinates(callback, onFailure);
        return;
    }

    console.log('Using geocoded coordinates');
    this.withGeocodeCoordinates(callback, onFailure);
};

WeatherProvider.prototype.withProviderData = function(lat, lon, force, onSuccess, onFailure) {
    console.log('This is the fallback implementation of withProviderData');
    onSuccess();
};

WeatherProvider.prototype.fetch = function(onSuccess, onFailure, force) {
    this.countryCode = null;
    this.locationMode = null;

    this.withCoordinates((function(lat, lon) {
        this.withCityName(lat, lon, (function(cityName, countryCode) {
            this.countryCode = countryCode;
            this.withSunEvents(lat, lon, (function(sunEvents) {
                this.withProviderData(lat, lon, force, (function() {
                    var payload;
                    // if `this` (the provider) contains valid weather details,
                    // then we can safely call this.getPayload()
                    if (this.hasValidData()) {
                        console.log('Lets get the payload for ' + cityName);
                        // Send to Pebble
                        this.cityName = cityName;
                        this.sunEvents = sunEvents;
                        payload = this.getPayload();
                        Pebble.sendAppMessage(
                            payload,
                            function(e) {
                                console.log('Weather info sent to Pebble successfully!');
                                onSuccess();
                            },
                            function(e) {
                                console.log('Error sending weather info to Pebble!');
                                onFailure(failure('app_message', 'nack'));
                            }
                        );
                    }
                    else {
                        console.log('Fetch cancelled: insufficient data.');
                        onFailure(failure('provider_data', 'invalid_data'));
                    }
                }).bind(this), function(providerFailure) {
                    onFailure(providerFailure || failure('provider_data', 'unknown_error'));
                });
            }).bind(this), function(sunFailure) {
                onFailure(sunFailure || failure('sun_events', 'unknown_error'));
            });
        }).bind(this), function(cityFailure) {
            onFailure(cityFailure || failure('reverse_geocode', 'unknown_error'));
        });
    }).bind(this), function(coordinateFailure) {
        onFailure(coordinateFailure || failure('coordinates', 'unknown_error'));
    });
};

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
};

WeatherProvider.prototype.getPayload = function() {
    // Get the rounded (integer) temperatures for those hours
    var temps = this.tempTrend.slice(0, this.numEntries).map(function(temperature) {
        return Math.round(temperature);
    });
    var precips = this.precipTrend.slice(0, this.numEntries).map(function(probability) {
        return Math.round(probability * 100);
    });
    var tempsIntView = new Int16Array(temps);
    var tempsByteArray = Array.prototype.slice.call(new Uint8Array(tempsIntView.buffer));
    var sunEventsIntView = new Int32Array(this.sunEvents.map(function(sunEvent) {
        return sunEvent.date.getTime() / 1000; // Seconds since epoch
    }));
    var sunEventsByteArray = Array.prototype.slice.call(new Uint8Array(sunEventsIntView.buffer));
    var payload = {
        TEMP_TREND_INT16: tempsByteArray,
        PRECIP_TREND_UINT8: precips, // Holds values within [0,100]
        FORECAST_START: this.startTime,
        NUM_ENTRIES: this.numEntries,
        CURRENT_TEMP: Math.round(this.currentTemp),
        CITY: this.cityName,
        // The first byte determines whether the list of events starts on a sunrise (0) or sunset (1)
        SUN_EVENTS: [this.sunEvents[0].type === 'sunrise' ? 0 : 1].concat(sunEventsByteArray)
    };
    return payload;
};

WeatherProvider.request = request;

module.exports = WeatherProvider;
