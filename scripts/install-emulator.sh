#!/usr/bin/env bash

set -euo pipefail

profile="dev"
emulator="${PEBBLE_EMULATOR:-basalt}"
install_args=()

if [[ "${1:-}" == "release" || "${1:-}" == "dev" ]]; then
  profile="$1"
  shift
fi

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
      install_args+=("$1")
      shift
      ;;
  esac
done

if [[ "$profile" == "dev" ]]; then
  mise run build -- dev
  pbw_path="build/forecaswatch2-dev.pbw"
else
  mise run build -- release
  pbw_path="build/forecaswatch2.pbw"
fi

if ((${#install_args[@]})); then
  pebble install "$pbw_path" --emulator "$emulator" "${install_args[@]}"
else
  pebble install "$pbw_path" --emulator "$emulator"
fi
