#!/usr/bin/env bash

set -euo pipefail

# Same as build.sh: npm_config_devdir is for node-gyp, not npm — see scripts/build.sh comment.
unset npm_config_devdir NPM_CONFIG_DEVDIR 2>/dev/null || true

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
const rn = pkg.releaseNotification || {};
const telemetryEndpoint = typeof process.env.TELEMETRY_ENDPOINT === 'string' ? process.env.TELEMETRY_ENDPOINT.trim() : '';
const hasEnabled = Object.prototype.hasOwnProperty.call(rn, 'enabled');
// Explicit enablement: only a literal boolean true turns this on.
const enabled = rn.enabled === true;
const title = typeof rn.title === 'string' ? rn.title.trim() : '';
const body = typeof rn.body === 'string' ? rn.body.trim() : '';

if (hasEnabled && typeof rn.enabled !== 'boolean') {
  throw new Error('releaseNotification.enabled must be a boolean (true or false) when present');
}

if ((title.length > 0 || body.length > 0) && !hasEnabled) {
  throw new Error('releaseNotification.title/body are set but releaseNotification.enabled is missing; set enabled to true or false explicitly');
}

if (enabled && title.length === 0) {
  throw new Error('releaseNotification.enabled is true but releaseNotification.title is empty or missing');
}

if (enabled && body.length === 0) {
  throw new Error('releaseNotification.enabled is true but releaseNotification.body is empty or missing');
}

pkg.telemetry = {
  enabled: telemetryEndpoint.length > 0,
  endpoint: telemetryEndpoint,
};

fs.writeFileSync('package.json', JSON.stringify(pkg, null, 2) + '\n');
"

printf 'prepared %s using %s\n' "$output_file" "$profile_file"
