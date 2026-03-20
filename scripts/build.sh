#!/usr/bin/env bash

set -euo pipefail

# node-gyp uses npm_config_devdir for its cache; npm 10+ warns because "devdir" is not an npm core
# config key (see https://github.com/nodejs/node-gyp/issues/3192). Cursor's sandbox and some other
# environments inject it — unset so Pebble's npm/npx steps don't spam the log.
unset npm_config_devdir NPM_CONFIG_DEVDIR 2>/dev/null || true

profile="dev"

if [[ "${1:-}" == "release" || "${1:-}" == "dev" ]]; then
  profile="$1"
  shift
fi

if [[ "${1:-}" == "--" ]]; then
  shift
fi

mise run prepare-package -- "$profile"
pebble build "$@"

if [[ "$profile" == "dev" ]]; then
  cp build/forecaswatch2.pbw build/forecaswatch2-dev.pbw
fi
