#!/usr/bin/env bash

set -euo pipefail

if [[ "${1:-}" == "--" ]]; then
  shift
fi

if (($#)); then
  pebble screenshot --cloudpebble "$@"
else
  output_dir="screenshot/tmp"
  timestamp="$(date +"%Y-%m-%dT%H-%M-%S")"
  output_path="${output_dir}/${timestamp}-cloud.png"
  mkdir -p "$output_dir"
  pebble screenshot "$output_path" --cloudpebble
fi
