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

if [[ "$profile" == "dev" ]]; then
  mise run build -- dev
  pebble install "build/forecaswatch2-dev.pbw" --cloudpebble "$@"
else
  mise run build -- release
  pebble install "build/forecaswatch2.pbw" --cloudpebble "$@"
fi
