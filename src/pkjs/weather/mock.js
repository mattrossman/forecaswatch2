var WeatherProvider = require('./provider.js');

var MOCK_SCENARIOS = {
    clearMorning: {
        startEpoch: 1772870400,
        currentTemp: 48,
        temps: [48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46],
        precipPct: [0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        sunEvents: [
            { type: 'sunrise', epoch: 1772883000 },
            { type: 'sunset', epoch: 1772923800 },
        ],
    },
    dryColdMorning: {
        startEpoch: 1772870400,
        currentTemp: 28,
        temps: [28, 27, 27, 26, 26, 27, 29, 32, 35, 37, 38, 39, 38, 36, 34, 33, 32, 31, 30, 29, 28, 28, 27, 27],
        precipPct: [0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0],
        sunEvents: [
            { type: 'sunrise', epoch: 1772883000 },
            { type: 'sunset', epoch: 1772923800 },
        ],
    },
    extremeCold: {
        startEpoch: 1772870400,
        currentTemp: -3,
        temps: [-9, -10, -10, -9, -8, -7, -7, -5, -4, -3, -2, -2, 0, 1, 3, 5, 5, 3, 1, -1, -2, -4, -6, -8],
        precipPct: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 4, 3, 1, 0, 0, 0, 0, 0, 0],
        precipMm: 8,
        precipType: 2,
        sunEvents: [
            { type: 'sunrise', epoch: 1772883000 },
            { type: 'sunset', epoch: 1772923800 },
        ],
    },
    extremeHot: {
        startEpoch: 1772870400,
        currentTemp: 105,
        temps: [100, 101, 100, 102, 103, 104, 103, 105, 104, 106, 105, 107, 108, 107, 106, 108, 107, 105, 104, 103, 102, 101, 100, 101],
        precipPct: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 4, 3, 1, 0, 0, 0, 0, 0, 0],
        sunEvents: [
            { type: 'sunrise', epoch: 1772883000 },
            { type: 'sunset', epoch: 1772923800 },
        ],
    },
    rainyNight: {
        startEpoch: 1772906400,
        currentTemp: 38,
        temps: [38, 38, 37, 37, 36, 36, 35, 35, 35, 34, 34, 34, 33, 33, 33, 33, 32, 32, 32, 32, 31, 31, 31, 31],
        precipPct: [40, 45, 55, 65, 70, 75, 80, 78, 70, 60, 50, 45, 40, 35, 30, 25, 20, 15, 10, 8, 6, 4, 2, 0],
        precipMm: 18,
        precipType: 1,
        sunEvents: [
            { type: 'sunset', epoch: 1772923800 },
            { type: 'sunrise', epoch: 1772968800 },
        ],
    },
    afternoonStorm: {
        startEpoch: 1772906400,
        currentTemp: 64,
        temps: [64, 66, 68, 70, 71, 72, 72, 71, 69, 67, 65, 63, 61, 59, 57, 55, 53, 51, 50, 49, 48, 47, 46, 45],
        precipPct: [5, 10, 15, 25, 40, 60, 80, 90, 85, 70, 55, 40, 30, 20, 12, 8, 6, 4, 3, 2, 2, 1, 1, 0],
        precipMm: 32,
        precipType: 1,
        sunEvents: [
            { type: 'sunset', epoch: 1772923800 },
            { type: 'sunrise', epoch: 1772968800 },
        ],
    },
    readme: {
        startEpoch: 1772906400,
        emuTime: '12:35:00',
        currentTemp: 52,
        temps: [52, 53, 54, 56, 57, 57, 56, 54, 52, 50, 48, 47, 46, 45, 45, 46, 47, 48, 49, 50, 50, 51, 51, 52],
        precipPct: [6, 6, 6, 6, 7, 8, 12, 18, 28, 40, 52, 62, 66, 60, 48, 34, 22, 14, 9, 7, 6, 5, 5, 5],
        sunEvents: [
            { type: 'sunrise', epoch: 1772886420 },
            { type: 'sunset', epoch: 1772923980 },
        ],
    },
};

function parseClockTime(value) {
    if (typeof value !== 'string') {
        return null;
    }

    var match = value.match(/^(\d{1,2}):(\d{2})(?::(\d{2}))?$/);
    if (!match) {
        return null;
    }

    var hour = parseInt(match[1], 10);
    var minute = parseInt(match[2], 10);
    var second = typeof match[3] === 'undefined' ? 0 : parseInt(match[3], 10);
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
        return null;
    }

    return {
        hour: hour,
        minute: minute,
        second: second,
    };
}

function epochForClockTimeToday(clockTime) {
    var now = new Date();
    var out = new Date(now.getTime());
    out.setHours(clockTime.hour);
    out.setMinutes(clockTime.minute);
    out.setSeconds(clockTime.second);
    out.setMilliseconds(0);
    return Math.floor(out.getTime() / 1000);
}

var MockProvider = function(devConfig) {
    this._super.call(this);
    this.name = 'Mock';
    this.id = 'mock';
    this.devConfig = devConfig || {};
};

MockProvider.prototype = Object.create(WeatherProvider.prototype);
MockProvider.prototype.constructor = MockProvider;
MockProvider.prototype._super = WeatherProvider;

MockProvider.prototype.withCoordinates = function(callback) {
    callback(0, 0);
};

MockProvider.prototype.withCityName = function(lat, lon, callback) {
    callback(this.devConfig.mockCity || 'Mock City');
};

MockProvider.prototype.resolveScenario = function() {
    var selectedName = this.devConfig.mockScenario;

    if (selectedName && Object.prototype.hasOwnProperty.call(MOCK_SCENARIOS, selectedName)) {
        return this.normalizeScenario(selectedName, MOCK_SCENARIOS[selectedName]);
    }

    var scenarioNames = Object.keys(MOCK_SCENARIOS);
    if (scenarioNames.length > 0) {
        return this.normalizeScenario(scenarioNames[0], MOCK_SCENARIOS[scenarioNames[0]]);
    }

    throw new Error('Mock provider has no built-in scenarios');
};

MockProvider.prototype.normalizeScenario = function(name, data) {
    var parsedClockTime;
    var baseStartEpoch;
    var epochShift;

    var normalized = {
        startEpoch: data.startEpoch,
        currentTemp: data.currentTemp,
        temps: data.temps,
        precipPct: data.precipPct,
        sunEvents: data.sunEvents,
    };

    if (typeof data.emuTime === 'number') {
        normalized.startEpoch = data.emuTime;
    }
    else {
        parsedClockTime = parseClockTime(data.emuTime);
        if (parsedClockTime) {
            normalized.startEpoch = epochForClockTimeToday(parsedClockTime);
        }
    }

    baseStartEpoch = data.startEpoch;
    if (typeof normalized.startEpoch === 'number' && typeof baseStartEpoch === 'number' && Array.isArray(data.sunEvents)) {
        epochShift = normalized.startEpoch - baseStartEpoch;
        normalized.sunEvents = data.sunEvents.map(function(event) {
            return {
                type: event.type,
                epoch: event.epoch + epochShift,
            };
        });
    }

    return {
        name: name,
        data: normalized,
    };
};

MockProvider.prototype.withSunEvents = function(lat, lon, callback) {
    var scenario = this.resolveScenario();
    var sunEvents = scenario.data.sunEvents || [];
    var normalizedSunEvents = sunEvents.map(function(event) {
        return {
            type: event.type,
            date: new Date(event.epoch * 1000),
        };
    });

    if (normalizedSunEvents.length >= 2) {
        callback(normalizedSunEvents.slice(0, 2));
        return;
    }

    var fallback = MOCK_SCENARIOS.clearMorning;
    console.log('[mock] Invalid sunEvents for scenario "' + scenario.name + '", falling back to clearMorning values');
    callback(fallback.sunEvents.map(function(event) {
        return {
            type: event.type,
            date: new Date(event.epoch * 1000),
        };
    }));
};

MockProvider.prototype.withProviderData = function(lat, lon, force, callback) {
    var scenario = this.resolveScenario();
    var data = scenario.data;

    console.log('[mock] Using scenario: ' + scenario.name);

    this.tempTrend = (data.temps || []).slice(0, this.numEntries);
    this.precipTrend = (data.precipPct || []).slice(0, this.numEntries).map(function(probabilityPercent) {
        return probabilityPercent / 100.0;
    });
    this.startTime = data.startEpoch;
    this.currentTemp = data.currentTemp;
    this.precipAmountTenthsMm = Math.round((data.precipMm || 0) * 10);
    this.precipType = data.precipType || 0;

    callback();
};

MockProvider.SCENARIOS = MOCK_SCENARIOS;

module.exports = MockProvider;
