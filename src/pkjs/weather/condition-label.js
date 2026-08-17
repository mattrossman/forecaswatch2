var WEATHER_COMPANY_LABELS = {
    0: 'Tornado',
    1: 'Tropical Storm',
    2: 'Hurricane',
    3: 'Severe Storm',
    4: 'T-storms',
    5: 'Wintry Mix',
    6: 'Rain/Sleet',
    7: 'Snow/Sleet',
    8: 'Frz Drizzle',
    9: 'Drizzle',
    10: 'Frz Rain',
    11: 'Showers',
    12: 'Showers',
    13: 'Flurries',
    14: 'Snow Showers',
    15: 'Blowing Snow',
    16: 'Snow',
    17: 'Hail',
    18: 'Sleet',
    19: 'Dust',
    20: 'Fog',
    21: 'Haze',
    22: 'Smoke',
    23: 'Blustery',
    24: 'Windy',
    25: 'Cold',
    26: 'Cloudy',
    27: 'Mostly Cloudy',
    28: 'Mostly Cloudy',
    29: 'Partly Cloudy',
    30: 'Partly Cloudy',
    31: 'Clear',
    32: 'Sunny',
    33: 'Fair',
    34: 'Fair',
    35: 'Rain/Hail',
    36: 'Hot',
    37: 'T-storms',
    38: 'T-storms',
    39: 'T-storms',
    40: 'Showers',
    41: 'Heavy Snow',
    42: 'Snow Showers',
    43: 'Heavy Snow',
    44: 'Partly Cloudy',
    45: 'T-showers',
    46: 'Snow Showers',
    47: 'T-showers'
};

/**
 * Convert an OpenWeather condition object to a short watch label.
 *
 * @param {Object|null} weather OpenWeather current weather object.
 * @returns {string} Short condition label.
 */
function fromOpenWeather(weather) {
    var id = weather && Number(weather.id);

    if (typeof id !== 'number' || !isFinite(id)) {
        return fromPhrase(weather && (weather.description || weather.main));
    }
    if (id >= 200 && id <= 232) {
        return 'T-storms';
    }
    if (id >= 300 && id <= 321) {
        return 'Drizzle';
    }
    if (id === 500) {
        return 'Light Rain';
    }
    if (id === 501) {
        return 'Rain';
    }
    if (id >= 502 && id <= 504) {
        return 'Heavy Rain';
    }
    if (id === 511) {
        return 'Frz Rain';
    }
    if (id >= 520 && id <= 531) {
        return 'Showers';
    }
    if (id === 600) {
        return 'Light Snow';
    }
    if (id === 601) {
        return 'Snow';
    }
    if (id === 602) {
        return 'Heavy Snow';
    }
    if (id >= 611 && id <= 613) {
        return 'Sleet';
    }
    if (id === 615 || id === 616) {
        return 'Wintry Mix';
    }
    if (id >= 620 && id <= 622) {
        return 'Snow Showers';
    }

    switch (id) {
        case 701: return 'Mist';
        case 711: return 'Smoke';
        case 721: return 'Haze';
        case 731: return 'Dust Whirls';
        case 741: return 'Fog';
        case 751: return 'Sand';
        case 761: return 'Dust';
        case 762: return 'Volcanic Ash';
        case 771: return 'Squalls';
        case 781: return 'Tornado';
        case 800: return 'Clear';
        case 801: return 'Few Clouds';
        case 802: return 'Partly Cloudy';
        case 803: return 'Mostly Cloudy';
        case 804: return 'Overcast';
        default: return fromPhrase(weather.description || weather.main);
    }
}

/**
 * Convert a Weather Company current observation to a short watch label.
 *
 * @param {Object|null} current Weather Company current observation.
 * @returns {string} Short condition label.
 */
function fromWeatherCompany(current) {
    var code = current && Number(current.iconCode);
    var phraseLabel = fromPhrase(current && (current.wxPhraseShort || current.wxPhraseMedium || current.wxPhraseLong));

    if (phraseLabel !== 'Unknown') {
        return phraseLabel;
    }
    if (typeof code === 'number' && isFinite(code) && Object.prototype.hasOwnProperty.call(WEATHER_COMPANY_LABELS, code)) {
        return WEATHER_COMPANY_LABELS[code];
    }

    return 'Unknown';
}

/**
 * Reduce an unexpected provider phrase to the fixed short-label vocabulary.
 *
 * @param {*} phrase Provider condition phrase.
 * @returns {string} Short condition label.
 */
function fromPhrase(phrase) {
    var value = typeof phrase === 'string' ? phrase.toLowerCase() : '';

    if (value.indexOf('tornado') !== -1) return 'Tornado';
    if (value.indexOf('hurricane') !== -1) return 'Hurricane';
    if (value.indexOf('tropical storm') !== -1) return 'Tropical Storm';
    if (value.indexOf('thunder') !== -1 || value.indexOf('t-storm') !== -1) return 'T-storms';
    if (value.indexOf('t-shower') !== -1) return 'T-showers';
    if (value.indexOf('freezing drizzle') !== -1 || value.indexOf('frz drizzle') !== -1) return 'Frz Drizzle';
    if (value.indexOf('freezing rain') !== -1 || value.indexOf('frz rain') !== -1) return 'Frz Rain';
    if (value.indexOf('rain and snow') !== -1 || value.indexOf('rain/snow') !== -1 || value.indexOf('wintry') !== -1) return 'Wintry Mix';
    if (value.indexOf('snow shower') !== -1) return 'Snow Showers';
    if (value.indexOf('heavy snow') !== -1) return 'Heavy Snow';
    if (value.indexOf('light snow') !== -1) return 'Light Snow';
    if (value.indexOf('blowing snow') !== -1) return 'Blowing Snow';
    if (value.indexOf('snow') !== -1) return 'Snow';
    if (value.indexOf('sleet') !== -1) return 'Sleet';
    if (value.indexOf('shower') !== -1) return 'Showers';
    if (value.indexOf('drizzle') !== -1) return 'Drizzle';
    if (value.indexOf('heavy rain') !== -1) return 'Heavy Rain';
    if (value.indexOf('light rain') !== -1) return 'Light Rain';
    if (value.indexOf('rain') !== -1) return 'Rain';
    if (value.indexOf('mostly cloudy') !== -1 || value === 'm cloudy') return 'Mostly Cloudy';
    if (value.indexOf('partly cloudy') !== -1 || value === 'p cloudy') return 'Partly Cloudy';
    if (value.indexOf('few cloud') !== -1) return 'Few Clouds';
    if (value.indexOf('overcast') !== -1) return 'Overcast';
    if (value.indexOf('cloud') !== -1) return 'Cloudy';
    if (value.indexOf('clear') !== -1) return 'Clear';
    if (value.indexOf('sunny') !== -1) return 'Sunny';
    if (value.indexOf('fair') !== -1) return 'Fair';
    if (value.indexOf('fog') !== -1) return 'Fog';
    if (value.indexOf('mist') !== -1) return 'Mist';
    if (value.indexOf('haze') !== -1) return 'Haze';
    if (value.indexOf('smoke') !== -1) return 'Smoke';
    if (value.indexOf('dust') !== -1) return 'Dust';
    if (value.indexOf('sand') !== -1) return 'Sand';
    if (value.indexOf('ash') !== -1) return 'Volcanic Ash';
    if (value.indexOf('squall') !== -1) return 'Squalls';
    if (value.indexOf('hail') !== -1) return 'Hail';
    if (value.indexOf('wind') !== -1) return 'Windy';
    if (value.indexOf('cold') !== -1) return 'Cold';
    if (value.indexOf('hot') !== -1) return 'Hot';

    return 'Unknown';
}

module.exports = {
    fromOpenWeather: fromOpenWeather,
    fromWeatherCompany: fromWeatherCompany
};
