#!/usr/bin/env bash

set -euo pipefail

profile="${1:-dev}"

if [[ "$profile" != "release" && "$profile" != "dev" ]]; then
  printf 'usage: %s [release|dev]\n' "$0" >&2
  exit 1
fi

template_file="package.template.json"
profile_file="profiles/package.${profile}.json"
output_file="package.json"

if [[ ! -f "$template_file" ]]; then
  printf 'missing template: %s\n' "$template_file" >&2
  exit 1
fi

if [[ ! -f "$profile_file" ]]; then
  printf 'missing profile: %s\n' "$profile_file" >&2
  exit 1
fi

npx --yes mustache "$profile_file" "$template_file" > "$output_file"
node -e "
const fs = require('fs');
const pkg = JSON.parse(fs.readFileSync('package.json', 'utf8'));
delete pkg.releaseNotification;

const manifestPath = 'release-notifications.json';
let manifest = {};
if (fs.existsSync(manifestPath)) {
  try {
    manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8'));
  } catch (e) {
    throw new Error('release-notifications.json: invalid JSON (' + e.message + ')');
  }
}
if (manifest === null || typeof manifest !== 'object' || Array.isArray(manifest)) {
  throw new Error('release-notifications.json must be a JSON object');
}

const ver = typeof pkg.version === 'string' ? pkg.version : '';
const entry = ver && Object.prototype.hasOwnProperty.call(manifest, ver) ? manifest[ver] : undefined;
if (entry !== undefined) {
  if (entry === null || typeof entry !== 'object' || Array.isArray(entry)) {
    throw new Error('release-notifications.json[' + JSON.stringify(ver) + '] must be an object with title and body');
  }
  const title = typeof entry.title === 'string' ? entry.title.trim() : '';
  const body = typeof entry.body === 'string' ? entry.body.trim() : '';
  if (title.length === 0) {
    throw new Error('release-notifications.json[' + JSON.stringify(ver) + '].title must be a non-empty string');
  }
  if (body.length === 0) {
    throw new Error('release-notifications.json[' + JSON.stringify(ver) + '].body must be a non-empty string');
  }
  pkg.releaseNotification = { enabled: true, title, body };
}

const telemetryEndpoint = typeof process.env.TELEMETRY_ENDPOINT === 'string' ? process.env.TELEMETRY_ENDPOINT.trim() : '';

pkg.telemetry = {
  enabled: telemetryEndpoint.length > 0,
  endpoint: telemetryEndpoint,
};

fs.writeFileSync('package.json', JSON.stringify(pkg, null, 2) + '\n');
"

printf 'prepared %s using %s\n' "$output_file" "$profile_file"
