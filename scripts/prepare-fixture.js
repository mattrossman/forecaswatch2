#!/usr/bin/env node

const fs = require('fs');
const path = require('path');

const fixtureName = (process.env.FIXTURE || '').trim();
const outputPath = path.join('src', 'pkjs', 'active-fixture.generated.js');

function fail(message) {
  console.error(message);
  process.exit(1);
}

function writeFixtureModule(value) {
  fs.writeFileSync(
    outputPath,
    'module.exports = ' + JSON.stringify(value, null, 2) + ';\n'
  );
}

/**
 * Fail unless a fixture time field is an integer in range.
 *
 * @param {string} pathLabel Human-readable fixture path.
 * @param {number} value Time field value.
 * @param {number} min Minimum allowed value.
 * @param {number} max Maximum allowed value.
 */
function assertIntInRange(pathLabel, value, min, max) {
  if (!Number.isInteger(value) || value < min || value > max) {
    fail(pathLabel + ' must be an integer from ' + min + '-' + max);
  }
}

/**
 * Convert local fixture fields anchored to watch.now into Unix seconds.
 *
 * @param {Object} watchNow Fixture watch.now value.
 * @param {Object} overrides Local date/time overrides.
 * @returns {number} Unix seconds.
 */
function dateFromWatchNow(watchNow, overrides) {
  const date = new Date(
    watchNow.year,
    watchNow.month - 1,
    watchNow.day + (overrides.dayOffset || 0),
    overrides.hour || 0,
    overrides.minute || 0,
    overrides.second || 0,
    0
  );

  return Math.floor(date.getTime() / 1000);
}

/**
 * Normalize readable fixture weather fields into the runtime shape.
 *
 * @param {Object} fixture Parsed fixture.
 */
function normalizeWeather(fixture) {
  const watchNow = fixture && fixture.watch && fixture.watch.now;
  const weather = fixture && fixture.weather;

  if (!watchNow || !weather) {
    return;
  }

  if (typeof weather.startHour === 'number') {
    assertIntInRange('weather.startHour', weather.startHour, 0, 23);
    weather.startEpoch = dateFromWatchNow(watchNow, {
      dayOffset: weather.startDayOffset || 0,
      hour: weather.startHour,
      minute: 0,
      second: 0,
    });
    delete weather.startHour;
    delete weather.startDayOffset;
  }

  if (Array.isArray(weather.sunEvents)) {
    weather.sunEvents = weather.sunEvents.map((event) => {
      if (typeof event.epoch === 'number') {
        return event;
      }

      assertIntInRange('weather.sunEvents.hour', event.hour, 0, 23);
      assertIntInRange('weather.sunEvents.minute', event.minute || 0, 0, 59);
      return {
        type: event.type,
        epoch: dateFromWatchNow(watchNow, {
          dayOffset: event.dayOffset || 0,
          hour: event.hour,
          minute: event.minute || 0,
          second: event.second || 0,
        }),
      };
    });
  }
}

if (fixtureName === '') {
  writeFixtureModule(null);
  process.exit(0);
}

if (!/^[a-z0-9][a-z0-9-]*$/.test(fixtureName)) {
  fail('FIXTURE must be a fixture slug like "readme" or "rainy-night"');
}

const fixturePath = path.join('fixtures', fixtureName + '.json');
if (!fs.existsSync(fixturePath)) {
  fail('Fixture not found: ' + fixturePath);
}

const fixture = JSON.parse(fs.readFileSync(fixturePath, 'utf8'));
fixture.name = fixtureName;
normalizeWeather(fixture);
writeFixtureModule(fixture);
