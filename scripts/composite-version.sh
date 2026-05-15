#!/bin/bash

set -euo pipefail

if [ "$#" -ne 1 ]; then
  echo "Usage: mise composite <version>"
  echo "Example: mise composite v1.33.0"
  exit 64
fi

version="$1"
raw_dir="screenshot/$version/raw"
composite_dir="screenshot/$version/composite"

if [ ! -d "$raw_dir" ]; then
  echo "Missing raw screenshot directory: $raw_dir"
  exit 66
fi

mkdir -p "$composite_dir"

composite_platform() {
  local platform="$1"
  local frame="$2"
  local output_name="$3"
  local raw="$raw_dir/$platform.png"

  if [ ! -f "$raw" ]; then
    return
  fi

  echo "Compositing $raw -> $composite_dir/$output_name.png"
  screenshot/composite_svg.sh "$frame" "$raw" "$composite_dir/$output_name.png"
}

composite_platform "basalt" "pebble-time-red" "pebble-time-red"
composite_platform "diorite" "pebble2-duo-white" "pebble2-duo-white"
composite_platform "flint" "pebble2-duo-white" "pebble2-duo-white"
composite_platform "emery" "pebble-time2-red" "pebble-time2-red"
