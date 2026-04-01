#!/usr/bin/env bash

set -euo pipefail

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk_version_file="$repo_root/pebble-sdk-version"

if [[ ! -f "$sdk_version_file" ]]; then
  echo "Missing Pebble SDK version file: $sdk_version_file" >&2
  exit 1
fi

sdk_version="$(<"$sdk_version_file")"
sdk_version="${sdk_version//$'\r'/}"

if [[ -z "$sdk_version" ]]; then
  echo "Empty Pebble SDK version file: $sdk_version_file" >&2
  exit 1
fi

if pebble sdk list | grep -qE "^${sdk_version}( \(active\))?$"; then
  pebble sdk activate "$sdk_version"
else
  pebble sdk install "$sdk_version"
  pebble sdk activate "$sdk_version"
fi
