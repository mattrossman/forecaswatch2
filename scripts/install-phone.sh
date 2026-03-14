#!/usr/bin/env bash

set -euo pipefail

ip="${IP:-}"
profile="dev"
install_args=()
ip_set_from_arg=0

while (($#)); do
  case "$1" in
    release|dev)
      profile="$1"
      shift
      ;;
    --)
      shift
      install_args+=("$@")
      break
      ;;
    -*)
      install_args+=("$1")
      shift
      ;;
    *)
      if [[ $ip_set_from_arg -eq 0 ]]; then
        ip="$1"
        ip_set_from_arg=1
      else
        install_args+=("$1")
      fi
      shift
      ;;
  esac
done

if [[ -z "$ip" ]]; then
  printf 'usage: %s [phone-ip] [release|dev] [-- pebble-install-args...]\n' "$0" >&2
  printf 'or set IP in environment and run: %s [release|dev] [pebble-install-args...]\n' "$0" >&2
  exit 1
fi

if [[ "$profile" == "dev" ]]; then
  mise run build -- dev
  if ((${#install_args[@]})); then
    pebble install build/forecaswatch2-dev.pbw --phone "$ip" "${install_args[@]}"
  else
    pebble install build/forecaswatch2-dev.pbw --phone "$ip"
  fi
else
  mise run build -- release
  if ((${#install_args[@]})); then
    pebble install build/forecaswatch2.pbw --phone "$ip" "${install_args[@]}"
  else
    pebble install build/forecaswatch2.pbw --phone "$ip"
  fi
fi
