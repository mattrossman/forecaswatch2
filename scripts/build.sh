#!/usr/bin/env bash

set -euo pipefail

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
