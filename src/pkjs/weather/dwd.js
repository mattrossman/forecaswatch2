var WeatherProvider = require('./provider.js');
var request = WeatherProvider.request;

var BRIGHTSKY_BASE = 'https://api.brightsky.dev';
var MAX_DIST_METERS = 500000;
var FORECAST_HOURS = 24;

/**
 * Convert Celsius to Fahrenheit. Returns null for non-numeric input.
 *
 * @param {number|null} celsius Temperature in Celsius.
 * @returns {number|null} Temperature in Fahrenheit, or null.
 */
function celsiusToFahrenheit(celsius) {
    if (typeof celsius !== 'number') {
        return null;
    }
    return celsius * 9 / 5 + 32;
}

/**
 * Compute the ISO 8601 timestamp now and now+24h (UTC, second precision).
 *
 * @returns {{ start: string, end: string }} ISO timestamps.
 */
function forecastWindow() {
    var now = new Date();
    var end = new Date(now.getTime() + FORECAST_HOURS * 60 * 60 * 1000);
    return {
        start: now.toISOString(),
        end: end.toISOString()
    };
}

var DwdProvider = function() {
    this._super.call(this);
    this.name = 'Brightsky (Deutscher Wetterdienst)';
    this.id = 'dwd';
};

DwdProvider.prototype = Object.create(WeatherProvider.prototype);
DwdProvider.prototype.constructor = DwdProvider;
DwdProvider.prototype._super = WeatherProvider;

/**
 * Fetch the Brightsky hourly forecast.
 *
 * @param {number} lat Latitude.
 * @param {number} lon Longitude.
 * @param {Function} callback Receives the weather[] array.
 * @param {Function} onFailure Receives a {stage, code} failure object.
 * @returns {void}
 */
DwdProvider.prototype.withDwdForecast = function(lat, lon, callback, onFailure) {
    var timeWindow = forecastWindow();
    // No units parameter: Brightsky's default returns °C for temperature.
    // Passing units=si would return Kelvin (verified via live probe).
    var url = BRIGHTSKY_BASE + '/weather'
        + '?lat=' + lat
        + '&lon=' + lon
        + '&date=' + encodeURIComponent(timeWindow.start)
        + '&last_date=' + encodeURIComponent(timeWindow.end)
        + '&max_dist=' + MAX_DIST_METERS;

    console.log('Requesting ' + url);

    request(
        url,
        'GET',
        (function(response) {
            var body;
            try {
                body = JSON.parse(response);
            }
            catch (ex) {
                onFailure({ stage: 'provider_data', code: 'dwd_forecast_parse_error' });
                return;
            }
            if (!body || !Array.isArray(body.weather)) {
                onFailure({ stage: 'provider_data', code: 'dwd_forecast_missing_fields' });
                return;
            }
            callback(body.weather);
        }).bind(this),
        function(error) {
            console.log('[!] DWD forecast request failed: ' + JSON.stringify(error));
            onFailure({ stage: 'provider_data', code: 'dwd_forecast_' + error.code });
        }
    );
};

/**
 * Fetch the Brightsky current observation.
 *
 * @param {number} lat Latitude.
 * @param {number} lon Longitude.
 * @param {Function} callback Receives the current temperature in Fahrenheit.
 * @param {Function} onFailure Receives a {stage, code} failure object.
 * @returns {void}
 */
DwdProvider.prototype.withDwdCurrent = function(lat, lon, callback, onFailure) {
    // No units parameter: see withDwdForecast for rationale.
    var url = BRIGHTSKY_BASE + '/current_weather'
        + '?lat=' + lat
        + '&lon=' + lon
        + '&max_dist=' + MAX_DIST_METERS;

    console.log('Requesting ' + url);

    request(
        url,
        'GET',
        function(response) {
            var body;
            var tempC;
            var tempF;
            try {
                body = JSON.parse(response);
            }
            catch (ex) {
                onFailure({ stage: 'provider_data', code: 'dwd_current_parse_error' });
                return;
            }
            if (!body || !body.weather || typeof body.weather.temperature !== 'number') {
                onFailure({ stage: 'provider_data', code: 'dwd_current_missing_fields' });
                return;
            }
            tempC = body.weather.temperature;
            tempF = celsiusToFahrenheit(tempC);
            callback(tempF);
        },
        function(error) {
            console.log('[!] DWD current request failed: ' + JSON.stringify(error));
            onFailure({ stage: 'provider_data', code: 'dwd_current_' + error.code });
        }
    );
};

// ============== IMPORTANT OVERRIDE ================
DwdProvider.prototype.withProviderData = function(lat, lon, force, onSuccess, onFailure) {
    // onSuccess expects that this.hasValidData() will be true.
    // The 30-minute cross-fetch cache lives in index.js (needRefresh); force is
    // unused here because DWD has no intra-fetch dedup target.

    this.withDwdForecast(lat, lon, (function(hourly) {
        var nonNullTemps;

        if (hourly.length < FORECAST_HOURS) {
            onFailure({ stage: 'provider_data', code: 'dwd_forecast_short' });
            return;
        }

        nonNullTemps = hourly.filter(function(entry) {
            return typeof entry.temperature === 'number';
        });
        if (nonNullTemps.length === 0) {
            onFailure({ stage: 'provider_data', code: 'dwd_forecast_all_null' });
            return;
        }

        this.withDwdCurrent(lat, lon, (function(currentTempF) {
            this.tempTrend = hourly.map(function(entry) {
                var converted = celsiusToFahrenheit(entry.temperature);
                return converted === null ? 0 : converted;
            });
            this.precipTrend = hourly.map(function(entry) {
                var probability = entry.precipitation_probability;
                return (typeof probability === 'number' ? probability : 0) / 100;
            });
            this.rainTrend = hourly.map(function(entry) {
                return typeof entry.precipitation === 'number' ? entry.precipitation : 0;
            });
            this.startTime = Math.floor(Date.parse(hourly[0].timestamp) / 1000);
            this.currentTemp = currentTempF;
            onSuccess();
        }).bind(this), onFailure);
    }).bind(this), onFailure);
};

module.exports = DwdProvider;
