#!/usr/bin/env bash

set -euo pipefail

profile="${1:-}"

if [[ -z "$profile" ]]; then
  printf 'usage: %s <release|dev>\n' "$0" >&2
  exit 1
fi

template_file="package.template.json"
profile_file="profiles/package.${profile}.json"
output_file="package.json"

if [[ ! -f "$template_file" ]]; then
  printf 'missing template: %s\n' "$template_file" >&2
  exit 1
fi

if [[ ! -f "$profile_file" ]]; then
  printf 'missing profile: %s\n' "$profile_file" >&2
  exit 1
fi

npx --yes mustache "$profile_file" "$template_file" > "$output_file"
node -e "JSON.parse(require('fs').readFileSync('package.json', 'utf8'))"

printf 'prepared %s using %s\n' "$output_file" "$profile_file"
