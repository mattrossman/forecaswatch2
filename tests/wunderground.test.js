const test = require('node:test');
const assert = require('node:assert/strict');
const WeatherProvider = require('../src/pkjs/weather/provider.js');

let requests = [];
let responseForRequest;
let useLiveRequests = false;
WeatherProvider.request = function(url, method, onSuccess, onFailure) {
    if (useLiveRequests) {
        fetch(url, { method, signal: AbortSignal.timeout(15000) })
            .then(response => {
                if (!response.ok) {
                    onFailure({ code: 'status_' + response.status });
                    return;
                }
                return response.text().then(onSuccess);
            })
            .catch(() => onFailure({ code: 'network_error' }));
        return;
    }
    requests.push(url);
    const response = responseForRequest(url);
    if (response.error) onFailure(response.error);
    else onSuccess(response.body);
};
const WundergroundProvider = require('../src/pkjs/weather/wunderground.js');

function resetStorage(initialKey) {
    let key = initialKey;
    global.localStorage = {
        getItem: () => key,
        setItem: (name, value) => { key = value; },
        removeItem: () => { key = null; }
    };
    requests = [];
}

test('extracts a key from the older homepage format', () => {
    resetStorage(null);
    responseForRequest = () => ({ body: 'observations/current?apiKey=abc123' });
    let extracted;
    new WundergroundProvider().withApiKey(value => { extracted = value; }, assert.fail);
    assert.equal(extracted, 'abc123');
});

test('clears a rejected cached key without retrying', () => {
    resetStorage('expired');
    responseForRequest = () => ({ error: { code: 'status_401' } });
    let failure;
    new WundergroundProvider().withProviderData(40, -74, false, assert.fail, error => { failure = error; });
    assert.equal(failure.code, 'wu_current_status_401');
    assert.equal(requests.length, 1);
    assert.equal(localStorage.getItem('wundergroundApiKey'), null);
});

test('clears a key rejected by the forecast request', () => {
    resetStorage('expired');
    responseForRequest = url => url.includes('/observations/current')
        ? { body: '{"temperature":72}' }
        : { error: { code: 'status_401' } };
    let failure;
    new WundergroundProvider().withProviderData(40, -74, false, assert.fail, error => { failure = error; });
    assert.equal(failure.code, 'wu_forecast_status_401');
    assert.equal(requests.length, 2);
    assert.equal(localStorage.getItem('wundergroundApiKey'), null);
});

test('homepage key works with current conditions and forecast', async () => {
    resetStorage(null);
    useLiveRequests = true;
    const originalLog = console.log;
    console.log = () => {};
    try {
        const provider = new WundergroundProvider();
        await new Promise((resolve, reject) => {
            provider.withProviderData(40.7128, -74.0060, false, resolve, reject);
        });
        assert.equal(typeof localStorage.getItem('wundergroundApiKey'), 'string');
        assert.equal(typeof provider.currentTemp, 'number');
        assert.ok(provider.tempTrend.length > 0);
    } finally {
        console.log = originalLog;
        useLiveRequests = false;
    }
});
