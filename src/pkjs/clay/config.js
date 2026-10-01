var meta = require('../../../package.json');
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
        "type": "section",
        "items": [
            {
                "type": "heading",
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
                "type": "toggle",
                "label": "UV index",
                "messageKey": "showUv",
                "defaultValue": true,
                "description": "Show the hourly UV index as yellow bars along the top of the forecast graph, and the current UV next to the current temperature. Only the Weather Underground provider supports UV so far; support for the other providers is a work in progress."
            },
            {
                "type": "select",
                "label": "UV protection threshold",
                "messageKey": "uvThreshold",
                "defaultValue": "3",
                "description": "The first UV bar is drawn at this UV index, so you can see when in the day to start protecting your skin. Each block above it is one more UV point, up to the top of the graph, and hours below it show no bar. The EPA and WHO recommend sun protection (shade, SPF 15+ sunscreen, hat, sunglasses) from UV 3 (Moderate), the default. Scale: 1-2 Low, 3-5 Moderate, 6-7 High, 8-10 Very High, 11+ Extreme. Choose 1 to show every hour with any UV. The current UV next to the temperature is not affected.",
                "options": [
                    {
                        "label": "1",
                        "value": "1"
                    },
                    {
                        "label": "2",
                        "value": "2"
                    },
                    {
                        "label": "3",
                        "value": "3"
                    },
                    {
                        "label": "4",
                        "value": "4"
                    },
                    {
                        "label": "5",
                        "value": "5"
                    },
                    {
                        "label": "6",
                        "value": "6"
                    },
                    {
                        "label": "7",
                        "value": "7"
                    },
                    {
                        "label": "8",
                        "value": "8"
                    },
                    {
                        "label": "9",
                        "value": "9"
                    },
                    {
                        "label": "10",
                        "value": "10"
                    },
                    {
                        "label": "11",
                        "value": "11"
                    }
                ]
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
