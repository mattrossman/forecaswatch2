#!/bin/bash

set -euo pipefail

if [ "$#" -ne 3 ]; then
  echo "Usage: mise composite-screenshot <frame> <screenshot.png> <output.png>"
  echo "Frames: pebble-time-red, pebble2-duo-white, pebble-time2-red"
  exit 64
fi

frame="$1"
screenshot="$2"
output="$3"
shadow_pad="5"

case "$frame" in
  pebble-time-red)
    frame_svg="screenshot/frames/pebble-time-red.svg"
    screen_x="61.8"
    screen_y="117"
    screen_w="144"
    screen_h="168"
    screen_rx="0"
    ;;
  pebble2-duo-white)
    frame_svg="screenshot/frames/pebble2-duo-white.svg"
    screen_x="38.5"
    screen_y="119"
    screen_w="144"
    screen_h="168"
    screen_rx="0"
    ;;
  pebble-time2-red)
    frame_svg="screenshot/frames/pebble-time2-red.svg"
    screen_x="46.5"
    screen_y="102"
    screen_w="200"
    screen_h="228"
    screen_rx="0"
    ;;
  *)
    echo "Unknown frame: $frame"
    echo "Frames: pebble-time-red, pebble2-duo-white, pebble-time2-red"
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

shadow_filter_def='<filter id="forecaswatch-device-shadow" x="-12%" y="-12%" width="124%" height="124%"><feDropShadow dx="0" dy="2" stdDeviation="3" flood-color="#000000" flood-opacity="0.18"/></filter>'

awk \
  -v image="$screenshot_base64" \
  -v shadow_filter_def="$shadow_filter_def" \
  -v pad="$shadow_pad" \
  -v x="$screen_x" \
  -v y="$screen_y" \
  -v width="$screen_w" \
  -v height="$screen_h" \
  -v rx="$screen_rx" \
  '
    function nfmt(value) {
      return sprintf("%g", value)
    }

    function attr_value(svg, name, pattern, value) {
      pattern = name "=\"[^\"]+\""
      if (!match(svg, pattern)) {
        return ""
      }
      value = substr(svg, RSTART + length(name) + 2, RLENGTH - length(name) - 3)
      return value
    }

    function expand_root(svg, original, value, width, height, view_box) {
      value = attr_value(svg, "width")
      if (value != "") {
        original = "width=\"" value "\""
        width = value + (pad * 2)
        sub(original, "width=\"" nfmt(width) "\"", svg)
      }
      value = attr_value(svg, "height")
      if (value != "") {
        original = "height=\"" value "\""
        height = value + (pad * 2)
        sub(original, "height=\"" nfmt(height) "\"", svg)
      }
      value = attr_value(svg, "viewBox")
      if (value != "") {
        original = "viewBox=\"" value "\""
        split(value, box, " ")
        view_box = nfmt(box[1] - pad) " " nfmt(box[2] - pad) " " nfmt(box[3] + (pad * 2)) " " nfmt(box[4] + (pad * 2))
        sub(original, "viewBox=\"" view_box "\"", svg)
      }
      return svg
    }

    /<\/svg>[[:space:]]*$/ {
      $0 = expand_root($0)
      sub(/<\/defs>/, shadow_filter_def "</defs><g filter=\"url(#forecaswatch-device-shadow)\">")
      sub(/<\/svg>[[:space:]]*$/, "")
      print
      print "  </g>"
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
    {
      if (NR == 1) {
        $0 = expand_root($0)
      }
      sub(/<\/defs>/, shadow_filter_def "</defs><g filter=\"url(#forecaswatch-device-shadow)\">")
      print
    }
  ' "$frame_svg" > "$tmp_svg"

resvg "$tmp_svg" "$output"
