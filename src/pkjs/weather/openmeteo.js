var WeatherProvider = require('./provider.js');
var request = WeatherProvider.request;

// API docs: https://open-meteo.com/en/docs
// Data license: CC BY 4.0, attribution required (see config page).

var FORECAST_URL = 'https://api.open-meteo.com/v1/forecast';

// Variables requested from the forecast endpoint. To expose a new data point,
// append it here and map it in withProviderData().
var CURRENT_VARS = ['temperature_2m'];
var HOURLY_VARS = ['temperature_2m', 'precipitation_probability', 'precipitation'];

var OpenMeteoProvider = function() {
    this._super.call(this);
    this.name = 'Open-Meteo';
    this.id = 'openmeteo';
};

OpenMeteoProvider.prototype = Object.create(WeatherProvider.prototype);
OpenMeteoProvider.prototype.constructor = OpenMeteoProvider;
OpenMeteoProvider.prototype._super = WeatherProvider;

/**
 * Build the forecast request URL in the units the watch payload expects.
 *
 * @param {number|string} lat Latitude.
 * @param {number|string} lon Longitude.
 * @param {number} hours Number of hourly entries starting at the current hour.
 * @returns {string} Request URL.
 */
function buildForecastUrl(lat, lon, hours) {
    return FORECAST_URL
        + '?latitude=' + lat
        + '&longitude=' + lon
        + '&current=' + CURRENT_VARS.join(',')
        + '&hourly=' + HOURLY_VARS.join(',')
        + '&temperature_unit=fahrenheit'
        + '&precipitation_unit=inch'
        + '&timeformat=unixtime'
        + '&forecast_hours=' + hours;
}

/**
 * Check that a value is a finite number.
 *
 * @param {*} value Value to check.
 * @returns {boolean} True for finite numbers.
 */
function isFiniteNumber(value) {
    return typeof value === 'number' && isFinite(value);
}

/**
 * Replace missing entries (null for some models/regions) with zero.
 *
 * @param {Array<number|null>} values Hourly values.
 * @returns {number[]} Values with non-numbers replaced by 0.
 */
function numbersOrZero(values) {
    return values.map(function(value) {
        return isFiniteNumber(value) ? value : 0;
    });
}

/**
 * Check that the response carries every variable the watchface needs.
 *
 * current.temperature_2m is validated by the finite check in withProviderData().
 *
 * @param {Object} weatherData Parsed Open-Meteo response.
 * @returns {boolean} True when the hourly block is complete.
 */
function hasRequestedFields(weatherData) {
    var hourly;

    if (!weatherData || !weatherData.current || !weatherData.hourly) {
        return false;
    }

    hourly = weatherData.hourly;
    if (!Array.isArray(hourly.time) || hourly.time.length === 0) {
        return false;
    }

    return HOURLY_VARS.every(function(name) {
        return Array.isArray(hourly[name]) && hourly[name].length === hourly.time.length;
    });
}

/**
 * Request and validate the Open-Meteo forecast.
 *
 * @param {number|string} lat Latitude.
 * @param {number|string} lon Longitude.
 * @param {Function} callback Called with the parsed response.
 * @param {Function} onFailure Called with a {stage, code} failure.
 * @returns {void}
 */
OpenMeteoProvider.prototype.withOpenMeteoResponse = function(lat, lon, callback, onFailure) {
    var url = buildForecastUrl(lat, lon, this.numEntries);

    console.log('Requesting ' + url);

    request(
        url,
        'GET',
        function(response) {
            var weatherData;
            try {
                weatherData = JSON.parse(response);
            }
            catch (ex) {
                onFailure({ stage: 'provider_data', code: 'om_parse_error' });
                return;
            }

            if (!hasRequestedFields(weatherData)) {
                onFailure({ stage: 'provider_data', code: 'om_missing_fields' });
                return;
            }

            callback(weatherData);
        },
        function(error) {
            console.log('[!] Open-Meteo request failed: ' + JSON.stringify(error));
            onFailure({ stage: 'provider_data', code: 'om_' + error.code });
        }
    );
};

// ============== IMPORTANT OVERRIDE ================
// Sun events come from the base SunCalc implementation.

OpenMeteoProvider.prototype.withProviderData = function(lat, lon, force, onSuccess, onFailure) {
    // onSuccess expects that this.hasValidData() will be true
    this.withOpenMeteoResponse(lat, lon, (function(weatherData) {
        var current = weatherData.current;
        var hourly = weatherData.hourly;

        // A missing temperature would otherwise round to a bogus 0°F.
        if (!isFiniteNumber(current.temperature_2m) || !hourly.temperature_2m.every(isFiniteNumber)) {
            onFailure({ stage: 'provider_data', code: 'om_missing_fields' });
            return;
        }

        this.tempTrend = hourly.temperature_2m;
        this.precipTrend = numbersOrZero(hourly.precipitation_probability).map(function(percent) {
            return percent / 100.0;
        });
        this.precipAmountTrend = numbersOrZero(hourly.precipitation);
        this.startTime = hourly.time[0];
        this.currentTemp = current.temperature_2m;
        onSuccess();
    }).bind(this), onFailure);
};

module.exports = OpenMeteoProvider;
