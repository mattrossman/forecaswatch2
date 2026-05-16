var WeatherProvider = require('./provider.js');
var request = WeatherProvider.request;

var WundergroundProvider = function() {
    this._super.call(this);
    this.name = 'Weather Underground';
    this.id = 'wunderground';
};

WundergroundProvider.prototype = Object.create(WeatherProvider.prototype);
WundergroundProvider.prototype.constructor = WundergroundProvider;
WundergroundProvider.prototype._super = WeatherProvider;

WundergroundProvider.prototype.withWundergroundForecast = function(lat, lon, apiKey, callback, onFailure) {
    // callback(wundergroundResponse)
    var url = 'https://api.weather.com/v1/geocode/' + lat + '/' + lon + '/forecast/hourly/48hour.json?apiKey=' + apiKey + '&language=en-US';

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
                onFailure({ stage: 'provider_data', code: 'wu_forecast_parse_error' });
                return;
            }

            if (!weatherData || !Array.isArray(weatherData.forecasts) || weatherData.forecasts.length === 0) {
                onFailure({ stage: 'provider_data', code: 'wu_forecast_missing_fields' });
                return;
            }

            callback(weatherData.forecasts);
        },
        function(error) {
            onFailure({ stage: 'provider_data', code: 'wu_forecast_' + error.code });
        }
    );
};

WundergroundProvider.prototype.withWundergroundCurrent = function(lat, lon, apiKey, callback, onFailure) {
    // callback(wundergroundResponse)
    var url = 'https://api.weather.com/v3/wx/observations/current?language=en-US&units=e&format=json'
        + '&apiKey=' + apiKey
        + '&geocode=' + lat + ',' + lon;

    console.log('Requesting ' + url);

    request(
        url,
        'GET',
        (function(response) {
            var weatherData;
            try {
                weatherData = JSON.parse(response);
            }
            catch (ex) {
                onFailure({ stage: 'provider_data', code: 'wu_current_parse_error' });
                return;
            }

            if (!weatherData || typeof weatherData.temperature !== 'number') {
                onFailure({ stage: 'provider_data', code: 'wu_current_missing_fields' });
                return;
            }

            callback(weatherData.temperature);
        }).bind(this),
        function(error) {
            onFailure({ stage: 'provider_data', code: 'wu_current_' + error.code });
        }
    );
};

WundergroundProvider.prototype.clearApiKey = function() {
    localStorage.removeItem('wundergroundApiKey');
    console.log('Cleared API key');
};

WundergroundProvider.prototype.withApiKey = function(callback, onFailure) {
    // callback(apiKey)

    var apiKey = localStorage.getItem('wundergroundApiKey');
    var url = 'https://www.wunderground.com/';

    if (apiKey === null) {
        console.log('Fetching Weather Underground API key');

        request(
            url,
            'GET',
            function(response) {
                var match = response.match(/observations\/current\?apiKey=([a-z0-9]*)/);
                if (!match || !match[1]) {
                    onFailure({ stage: 'provider_data', code: 'wu_api_key_not_found' });
                    return;
                }

                apiKey = match[1];
                localStorage.setItem('wundergroundApiKey', apiKey);
                console.log('Fetched Weather Underground API key: ' + apiKey);
                callback(apiKey);
            },
            function(error) {
                onFailure({ stage: 'provider_data', code: 'wu_api_key_' + error.code });
            }
        );
    }
    else {
        console.log('Using saved API key for Weather Underground');
        callback(apiKey);
    }
};

// ============== IMPORTANT OVERRIDE ================

WundergroundProvider.prototype.withProviderData = function(lat, lon, force, onSuccess, onFailure) {
    // onSuccess expects that this.hasValidData() will be true

    if (force) {
        // In case the API key becomes invalid
        console.log('Clearing Weather Underground API key for forced update');
        this.clearApiKey();
    }

    this.withApiKey((function(apiKey) {
        this.withWundergroundCurrent(lat, lon, apiKey, (function(currentTemp) {
            this.withWundergroundForecast(lat, lon, apiKey, (function(forecast) {
                this.tempTrend = forecast.map(function(entry) {
                    return entry.temp;
                });
                this.precipTrend = forecast.map(function(entry) {
                    return entry.pop / 100.0;
                });
                this.windTrend = forecast.map(function(entry) {
                    // Try a few common field names used by providers
                    if (entry.wspd) {
                        if (typeof entry.wspd === 'object') {
                            return entry.wspd.english || entry.wspd.metric || 0;
                        }
                        return entry.wspd;
                    }
                    if (entry.wspd_mph) return entry.wspd_mph;
                    if (entry.wind_speed) return entry.wind_speed;
                    return 0;
                });
                this.startTime = forecast[0].fcst_valid;
                this.currentTemp = currentTemp;
                onSuccess();
            }).bind(this), onFailure);
        }).bind(this), onFailure);
    }).bind(this), onFailure);
};

module.exports = WundergroundProvider;
