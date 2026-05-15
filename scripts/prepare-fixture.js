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
writeFixtureModule(fixture);
