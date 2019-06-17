#!/bin/bash

if [ -n "$PEBBLE_PHONE" ]; then
    echo Attempting clean install to $PEBBLE_PHONE
else
    echo "you didn't configure the phone IP yet"
    exit 1
fi

pebble clean
pebble build
pebble install
