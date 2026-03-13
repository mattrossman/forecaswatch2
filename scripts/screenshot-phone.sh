#!/usr/bin/env bash

set -euo pipefail

ip="${IP:-}"

if [[ -n "${1:-}" ]]; then
  ip="$1"
  shift
fi

if [[ -z "$ip" ]]; then
  printf 'usage: %s <phone-ip> [-- pebble-screenshot-args...]\n' "$0" >&2
  printf 'or set IP in environment and run: %s\n' "$0" >&2
  exit 1
fi

if [[ "${1:-}" == "--" ]]; then
  shift
fi

if [[ "$#" -eq 0 ]]; then
  output_dir="screenshot/tmp"
  timestamp="$(date +"%Y-%m-%dT%H-%M-%S")"
  output_path="${output_dir}/${timestamp}.png"
  mkdir -p "$output_dir"
  pebble screenshot "$output_path" --phone "$ip"
else
  pebble screenshot --phone "$ip" "$@"
fi
