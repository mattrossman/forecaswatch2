#!/usr/bin/env bash

set -euo pipefail

# Activate the Pebble SDK version pinned in the repo before any build or install.

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

if ! pebble sdk activate "$sdk_version"; then
  pebble sdk install "$sdk_version"
  pebble sdk activate "$sdk_version"
fi
