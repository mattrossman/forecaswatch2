#!/usr/bin/env bash

set -euo pipefail

profile="dev"
emulator="${PEBBLE_EMULATOR:-basalt}"
install_args=()

read_dev_config_value() {
  local key="$1"
  node -e 'try { var cfg = require("./src/pkjs/dev-config.js"); var key = process.argv[1]; if (!Object.prototype.hasOwnProperty.call(cfg, key) || cfg[key] === null || typeof cfg[key] === "undefined") { process.exit(2); } process.stdout.write(String(cfg[key])); } catch (e) { process.exit(2); }' "$key"
}

read_mock_scenario_time() {
  node -e 'try {
    var cfg = require("./src/pkjs/dev-config.js");
    if (cfg.provider !== "mock") { process.exit(2); }
    var MockProvider = require("./src/pkjs/weather/mock.js");
    var scenarios = MockProvider.SCENARIOS || {};
    var names = Object.keys(scenarios);
    if (!names.length) { process.exit(2); }
    var selected = cfg.mockScenario;
    var scenarioName = Object.prototype.hasOwnProperty.call(scenarios, selected) ? selected : names[0];
    var scenario = scenarios[scenarioName] || {};
    if (typeof scenario.emuTime === "string" || typeof scenario.emuTime === "number") {
      process.stdout.write(String(scenario.emuTime));
      process.exit(0);
    }
    if (typeof scenario.startEpoch !== "number") { process.exit(2); }
    process.stdout.write(String(scenario.startEpoch));
  } catch (e) { process.exit(2); }'
}

apply_emulator_overrides() {
  local emu_time_format
  local emu_time

  if emu_time_format="$(read_dev_config_value emuTimeFormat)"; then
    :
  else
    emu_time_format="24h"
  fi

  if [[ "$emu_time_format" != "12h" && "$emu_time_format" != "24h" ]]; then
    echo "Invalid emuTimeFormat in src/pkjs/dev-config.js: $emu_time_format (expected 12h or 24h)" >&2
    exit 1
  fi

  pebble emu-time-format --emulator "$emulator" --format "$emu_time_format"

  if emu_time="$(read_dev_config_value emuTime)"; then
    :
  elif emu_time="$(read_mock_scenario_time)"; then
    :
  else
    emu_time="$(date +%s)"
  fi

  if [[ "$emu_time" =~ ^[0-9]{1,2}:[0-9]{2}$ ]]; then
    emu_time="${emu_time}:00"
  fi

  pebble emu-set-time --emulator "$emulator" "$emu_time"
}

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
    aplite|basalt|chalk|diorite|emery)
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

apply_emulator_overrides
