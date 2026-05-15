#!/bin/bash

set -euo pipefail

if [ "$#" -ne 3 ]; then
  echo "Usage: mise composite-screenshot <frame> <screenshot.png> <output.png>"
  echo "Frames: pebble-time-red, pebble2-white, core-time2-red"
  exit 64
fi

frame="$1"
screenshot="$2"
output="$3"

case "$frame" in
  pebble-time-red)
    frame_svg="screenshot/frames/pebble-time-red.svg"
    screen_x="61.8"
    screen_y="117.5"
    screen_w="144"
    screen_h="168"
    screen_rx="0"
    ;;
  pebble2-white)
    frame_svg="screenshot/frames/pebble2-white.svg"
    screen_x="38.5"
    screen_y="119"
    screen_w="144"
    screen_h="168"
    screen_rx="0"
    ;;
  core-time2-red)
    frame_svg="screenshot/frames/core-time2-red.svg"
    screen_x="46.5"
    screen_y="102"
    screen_w="200"
    screen_h="228"
    screen_rx="0"
    ;;
  *)
    echo "Unknown frame: $frame"
    echo "Frames: pebble-time-red, pebble2-white, core-time2-red"
    exit 64
    ;;
esac

if [ ! -f "$frame_svg" ]; then
  echo "Missing frame SVG: $frame_svg"
  exit 66
fi

if [ ! -f "$screenshot" ]; then
  echo "Missing screenshot: $screenshot"
  exit 66
fi

tmp_svg="$(mktemp "${TMPDIR:-/tmp}/forecaswatch-frame.XXXXXX")"
trap 'rm -f "$tmp_svg"' EXIT

screenshot_base64="$(base64 < "$screenshot" | tr -d '\n')"

awk \
  -v image="$screenshot_base64" \
  -v x="$screen_x" \
  -v y="$screen_y" \
  -v width="$screen_w" \
  -v height="$screen_h" \
  -v rx="$screen_rx" \
  '
    /<\/svg>[[:space:]]*$/ {
      sub(/<\/svg>[[:space:]]*$/, "")
      print
      print "  <defs>"
      print "    <clipPath id=\"forecaswatch-screen-clip\">"
      print "      <rect x=\"" x "\" y=\"" y "\" width=\"" width "\" height=\"" height "\" rx=\"" rx "\" ry=\"" rx "\"/>"
      print "    </clipPath>"
      print "  </defs>"
      print "  <image"
      print "    x=\"" x "\""
      print "    y=\"" y "\""
      print "    width=\"" width "\""
      print "    height=\"" height "\""
      print "    preserveAspectRatio=\"none\""
      print "    clip-path=\"url(#forecaswatch-screen-clip)\""
      print "    href=\"data:image/png;base64," image "\"/>"
      print "</svg>"
      next
    }
    { print }
  ' "$frame_svg" > "$tmp_svg"

resvg "$tmp_svg" "$output"
