#!/usr/bin/env bash

set -euo pipefail

emulator="${PEBBLE_EMULATOR:-basalt}"
screenshot_args=()

if [[ "${1:-}" == "--" ]]; then
  shift
fi

while (($#)); do
  case "$1" in
    --emulator)
      if [[ -z "${2:-}" ]]; then
        echo "Missing value for --emulator" >&2
        exit 1
      fi
      emulator="$2"
      shift 2
      ;;
    --emulator=*)
      emulator="${1#*=}"
      shift
      ;;
    aplite|basalt|chalk|diorite|emery|flint|gabbro)
      emulator="$1"
      shift
      ;;
    *)
      screenshot_args+=("$1")
      shift
      ;;
  esac
done

if ((${#screenshot_args[@]})); then
  pebble screenshot --emulator "$emulator" "${screenshot_args[@]}"
else
  output_dir="screenshot/tmp"
  timestamp="$(date +"%Y-%m-%dT%H-%M-%S")"
  output_path="${output_dir}/${timestamp}-${emulator}.png"
  mkdir -p "$output_dir"
  pebble screenshot "$output_path" --emulator "$emulator"
fi
