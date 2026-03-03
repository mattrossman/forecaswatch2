#!/usr/bin/env bash

set -euo pipefail

profile="release"

if [[ "${1:-}" == "release" || "${1:-}" == "dev" ]]; then
  profile="$1"
  shift
fi

if [[ "${1:-}" == "--" ]]; then
  shift
fi

if [[ "$profile" == "dev" ]]; then
  mise run build-dev
  pebble install "build/forecaswatch2-dev.pbw" --emulator basalt "$@"
else
  mise run build
  pebble install "build/forecaswatch2.pbw" --emulator basalt "$@"
fi
