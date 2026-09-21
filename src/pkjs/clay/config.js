var meta = require('../../../package.json');
var statsSlots = require('./stats-slots.js');
var versionLabel = "v" + meta.version + (meta.buildProfile === "dev" ? " (dev)" : "");

module.exports = [
    {
        "type": "heading",
        "defaultValue": "ForecasWatch2"
    },
    {
        "type": "text",
        "defaultValue": "Contribute on <a href=\"https://github.com/mattrossman/forecaswatch2\">GitHub!</a>"
    },
    {
        "type": "section",
        "items": [
            {
                "type": "heading",
                "defaultValue": "Time",
            },
            {
                "type": "toggle",
                "label": "Leading zero",
                "messageKey": "timeLeadingZero",
            },
            {
                "type": "toggle",
                "label": "Show AM/PM",
                "messageKey": "timeShowAmPm",
            },
            {
                "type": "select",
                "label": "Axis time format",
                "messageKey": "axisTimeFormat",
                "defaultValue": "24h",
                "description": "Tip: go to Settings > Date & Time > Time Format on your watch to change the main time format",
                "options": [
                    {
                        "label": "12h",
                        "value": "12h"
                    },
                    {
                        "label": "24h",
                        "value": "24h"
                    }
                ]
            },
            {
                "type": "select",
                "label": "Main time font",
                "messageKey": "timeFont",
                "defaultValue": "roboto",
                "options": [
                    {
                        "label": "Roboto",
                        "value": "roboto"
                    },
                    {
                        "label": "Leco",
                        "value": "leco"
                    },
                    {
                        "label": "Bitham",
                        "value": "bitham"
                    },
                ]
            },
            {
                "type": "color",
                "label": "Main time color",
                "messageKey": "colorTime",
                "defaultValue": "#FFFFFF",
                "sunlight": false,
                "capabilities": ["COLOR"]
            },
        ]
    },
    {
        // Gated on NOT_PLATFORM_APLITE rather than HEALTH: Clay's bundled
        // capability table predates flint, so a HEALTH gate would wrongly hide
        // this whole section on a Pebble 2 Duo.
        "type": "section",
        "capabilities": ["NOT_PLATFORM_APLITE"],
        "items": [
            {
                "type": "heading",
                "defaultValue": "Top band: calendar or health stats"
            },
            {
                "type": "select",
                "label": "Show in top band",
                "messageKey": "topBand",
                "defaultValue": "calendar",
                "description": "Choose \"Health stats grid\" to replace the 3 week calendar with steps, distance, heart rate and so on. The grid also takes over the month/battery row above the calendar. Pick the metrics per slot below.",
                "options": [
                    { "label": "3 week calendar", "value": "calendar" },
                    { "label": "Health stats grid", "value": "stats" }
                ]
            },
            {
                "type": "text",
                "id": "statsGridLayout",
                // Replaced in inject.js with the connected watch's grid size.
                "defaultValue": "2x2 grid"
            },
            {
                "type": "select",
                "label": "Slot 1",
                "messageKey": "statSlot1",
                "defaultValue": "distance",
                "options": statsSlots.SLOT_OPTIONS
            },
            {
                "type": "select",
                "label": "Slot 2",
                "messageKey": "statSlot2",
                "defaultValue": "heartRate",
                "options": statsSlots.SLOT_OPTIONS
            },
            {
                "type": "select",
                "label": "Slot 3",
                "messageKey": "statSlot3",
                "defaultValue": "steps",
                "options": statsSlots.SLOT_OPTIONS
            },
            {
                "type": "select",
                "label": "Slot 4",
                "messageKey": "statSlot4",
                "defaultValue": "calories",
                "options": statsSlots.SLOT_OPTIONS
            },
            {
                "type": "select",
                "label": "Slot 5",
                "messageKey": "statSlot5",
                "defaultValue": "active",
                "options": statsSlots.SLOT_OPTIONS
            },
            {
                "type": "select",
                "label": "Slot 6",
                "messageKey": "statSlot6",
                "defaultValue": "uvIndex",
                "options": statsSlots.SLOT_OPTIONS
            }
        ]
    },
    {
        "type": "section",
        "items": [
            {
                "type": "heading",
                // inject.js marks it inactive while the stats grid is selected.
                "id": "calendarHeading",
                "defaultValue": "Calendar",
            },
            {
                "type": "select",
                "label": "Start week on",
                "messageKey": "weekStartDay",
                "defaultValue": "sun",
                "options": [
                    {
                        "label": "Sunday",
                        "value": "sun"
                    },
                    {
                        "label": "Monday",
                        "value": "mon"
                    }
                ]
            },
            {
                "type": "select",
                "label": "First week to display",
                "messageKey": "firstWeek",
                "defaultValue": "prev",
                "options": [
                    {
                        "label": "Previous week",
                        "value": "prev"
                    },
                    {
                        "label": "Current week",
                        "value": "curr"
                    }
                ]
            },
            {
                "type": "color",
                "label": "Today highlight",
                "messageKey": "colorToday",
                "defaultValue": "#000000",
                "description": "Black (default) means match date color, any other value overrides this.",
                "sunlight": false,
                "capabilities": ["COLOR"]
            },
            {
                "type": "color",
                "label": "Sunday color",
                "messageKey": "colorSunday",
                "defaultValue": "#FF0055",
                "sunlight": false,
                "capabilities": ["COLOR"]
            },
            {
                "type": "color",
                "label": "Saturday color",
                "messageKey": "colorSaturday",
                "defaultValue": "#FF0055",
                "sunlight": false,
                "capabilities": ["COLOR"]
            },
            {
                "type": "color",
                "label": "US federal holidays color",
                "messageKey": "colorUSFederal",
                "defaultValue": "#FF0055",
                "description": "White means disable",
                "sunlight": false,
                "capabilities": ["COLOR"]
            },
        ]
    },
    {
        "type": "section",
        "items": [
            {
                "type": "heading",
                "defaultValue": "Weather"
            },
            {
                "type": "select",
                "defaultValue": "f",
                "messageKey": "temperatureUnits",
                "label": "Temperature Units",
                "options": [
                    {
                        "label": "°F",
                        "value": "f"
                    },
                    {
                        "label": "°C",
                        "value": "c"
                    }
                ]
            },
            {
                "type": "toggle",
                "label": "Day/night shading",
                "messageKey": "dayNightShading",
                "defaultValue": true,
                "description": "Show hatch shading between sunset and sunrise to distinguish day and night on the forecast graph."
            },
            {
                "type": "toggle",
                "label": "Precipitation amount",
                "messageKey": "precipAmountBars",
                "defaultValue": true,
                "description": "Show bars for the hourly precipitation amount. Shorter bars indicate lighter rain. Full-height bars indicate heavy rain above 0.75 in (19 mm) per hour."
            },
            {
                "type": "radiogroup",
                "label": "Provider",
                "messageKey": "provider",
                "defaultValue": "wunderground",
                "description": "Weather data by <a href='https://open-meteo.com/'>Open-Meteo.com</a>, licensed under <a href='https://creativecommons.org/licenses/by/4.0/'>CC BY 4.0</a>.",
                "options": [
                    {
                        "label": "Weather Underground",
                        "value": "wunderground"
                    },
                    {
                        "label": "OpenWeatherMap",
                        "value": "openweathermap"
                    },
                    {
                        "label": "Open-Meteo",
                        "value": "openmeteo"
                    }
                ]
            },
            {
                "type": "input",
                "label": "OpenWeatherMap API key",
                "messageKey": "owmApiKey",
                "description": "<a href='https://openweathermap.org/'>Register an OpenWeatherMap account</a> and paste your API key here"
            },
            {
                "type": "toggle",
                "label": "Force weather fetch",
                "messageKey": "fetch",
                "description": "Last successful fetch:<br><span id='lastFetchSpan'>Never :(</span><span id='lastAttemptBlock'></span>"
            },
            {
                "type": "input",
                "label": "Location override",
                "messageKey": "location",
                "description": "Example: \"Manhattan\" or \"123 Oak St Plainsville KY\".<br><a href=\"https://locationiq.com/demo\">Click here</a> to test out your location query.<br>To use GPS, leave this blank and ensure GPS is enabled on your device.",
                "attributes": {
                    "placeholder": "Using GPS",
                }
            }
        ]
    },
    {
        "type": "section",
        "items": [
            {
                "type": "heading",
                "defaultValue": "Misc"
            },
            {
                "type": "toggle",
                "label": "Show quiet time icon",
                "messageKey": "showQt",
                "defaultValue": true
            },
            {
                "type": "toggle",
                "label": "Vibrate on bluetooth disconnect",
                "messageKey": "vibe",
                "defaultValue": false
            },
            {
                "type": "select",
                "defaultValue": "both",
                "messageKey": "btIcons",
                "label": "Show icon for bluetooth",
                "options": [
                    {
                        "label": "Disconnected",
                        "value": "disconnected"
                    },
                    {
                        "label": "Connected",
                        "value": "connected"
                    },
                    {
                        "label": "Both",
                        "value": "both"
                    },
                    {
                        "label": "None",
                        "value": "none"
                    }
                ]
            },
            {
                "type": "toggle",
                "label": "Share anonymous telemetry",
                "messageKey": "telemetryEnabled",
                "defaultValue": true,
                "description": "<span style=\"color:#9aa0a6;font-size:0.82em;line-height:1.35;\">Share privacy-respecting weather telemetry to improve reliability and understand usage patterns. Learn more about what gets sent in the <a href=\"https://github.com/mattrossman/forecaswatch2#telemetry\">Telemetry section</a>.</span>"
            },
        ]
    },
    {
        "type": "submit",
        "defaultValue": "Save Settings"
    },
    {
        "type": "text",
        "defaultValue": versionLabel
    }
]
