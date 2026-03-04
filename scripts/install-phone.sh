#!/usr/bin/env bash

set -euo pipefail

ip="${IP:-}"
profile="dev"

if [[ -n "${1:-}" ]]; then
  if [[ ("$1" == "release" || "$1" == "dev") && -n "$ip" ]]; then
    profile="$1"
    shift
  else
    ip="$1"
    shift
  fi
fi

if [[ "${1:-}" == "release" || "${1:-}" == "dev" ]]; then
  profile="$1"
  shift
fi

if [[ -z "$ip" ]]; then
  printf 'usage: %s <phone-ip> [release|dev] [-- pebble-install-args...]\n' "$0" >&2
  printf 'or set IP in environment and run: %s [release|dev]\n' "$0" >&2
  exit 1
fi

if [[ "${1:-}" == "--" ]]; then
  shift
fi

if [[ "$profile" == "dev" ]]; then
  mise run build -- dev
  pebble install build/forecaswatch2-dev.pbw --phone "$ip" "$@"
else
  mise run build -- release
  pebble install build/forecaswatch2.pbw --phone "$ip" "$@"
fi
