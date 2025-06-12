![banner](https://i.imgur.com/IIr8MCp.png)

[![GitHub release (latest by date)](https://img.shields.io/github/v/release/genko/velowatch?label=latest&color=85C1E9&logo=data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAQAAABKfvVzAAAAAmJLR0QA/4ePzL8AAAC0SURBVDjLldLLCcJAFEbhP4oIE1xYhw1pA4JWYRGCVbhxoTshWIUNaAnBgB4XBiTJ3HncbALznXAZIpnDmHnnmSqAS470p2Fn8wrfNEzS+INX++ZS+J6CRZu4lGWWEgVPT2DsfmfFwbMSjiuxcXn8H5gX6Q+S+ZtRDofT7/uXRF5RSmJEncEliS2fDC5JrCPJjVn/lwglQx5M/NxMbO5NwnyQxHknSeOSxIaac+feo0lhn30BIXaN/u4MXmAAAAAASUVORK5CYII=)](https://github.com/genko/velowatch/releases/latest)
[![GitHub All Releases](https://img.shields.io/github/downloads/genko/velowatch/total?label=downloads&logo=data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAMAAADXqc3KAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAM1BMVEUAAAD///////////////v///z///z///z///v///v///v///v///v///3///z///z///xB67/PAAAAEHRSTlMAERwgPExhYouMjY6P1N7g8FyKngAAAF5JREFUKM/VjksOgCAMBR+IH0CB+59WEBpTPsYts+tM2hTIiLUgwFGhoKYN+xVxFFyatlx0qDhox3Cv32tm4FnR/CtL3lTvUmk8cCZvgW7p+lgG/j9yaZBP8KHBf4QbpMkNa908ZS8AAAAASUVORK5CYII=)](https://github.com/genko/velowatch/releases/latest/download/velowatch.pbw)
[![Platform](https://img.shields.io/badge/platform-basalt-red?logo=data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAQAAABKfvVzAAAAAmJLR0QA/4ePzL8AAAC1SURBVDjL1ZSxDoIwFEWbMOLE6qgYdlb5FUkMFEOMox8pJsJg/A4HmGqOkwZtoTRx8b7t5p6h6X1PCIMIiIgIxBQxo+GlGt8OHOhrb4t73D6AK944sONbxVg8p9OAjmwovuLBiRLZm5IKxdIMpMBacxNgYwYkEGtuDNyZuwEG3waEbsDR9Q3yb4AUSFw+LkRRadU4o1gMtSmj1crXsh3ra6EB+W8XyHlFhcCnfscvE46A7cw8Ab9fQsIsgqKzAAAAAElFTkSuQmCC)](https://developer.rebble.io/developer.pebble.com/guides/tools-and-resources/hardware-information/index.html)
[![GitHub](https://img.shields.io/github/license/genko/velowatch?color=blue&logo=data%3Aimage%2Fpng%3Bbase64%2CiVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAACXBIWXMAAAsTAAALEwEAmpwYAAAB7ElEQVRIie2TPWsUURSGnxsWjLosIgTJLpL8AbWwUKwjgpg%2FYEAQxCiCSLKgsAppBG38AFNolU72P4hlrLZQEwsVxSD4AWmyKFmLPBY5wWGys9ktBAtfGJh73o9z7p078K9ATeqkOqWmv9Fg0j84269vaIAelcx7u19TaSeBOgocBYaBlxE%2Bop4BWimlL32PqJ5Sj6hVdVZtqRsWYyM0dbUW3tNF4eM9gt6py5n1ctSKMF7U5IL6NkRN9ZxaC24uEzAXtVponkb9vTrd64h2qSshvpzjtjXIcBej%2FlkdznL5WzQNjALPgUZeXDQUcCs8B4BLRcI96lf1cXyPjjqz0w7Ua6EdU59Ext5uDa6rP9VqrB%2Bq39VyUQO1rH5TH8S6Ghk38uEVdVW9k6mNqG210aNBQ11TRzK%2Bu5FVASjFywKwH9id%2B4AfgLr6usuOJ4E68BG4om5Rw5G1oJ4vAevAsSCvbju7TYwV1PbFc7gLfxxYH0op%2FQJOAktdRB3gNvCoCzcfXKcLtwRMpJQ6JYCU0hvgkHoipqkAn4BnKaVVgMwREB6Bm%2Bp9YCJ2tAa8SiktbulKOdMisMgAiAGag3i2QZ2JG7WFdvYf6TlAj9CDwBSbt2IWKOckbeAe8ANoppRWBp26Zf94MVD4f2TxGzyXVgXKldOBAAAAAElFTkSuQmCC)](https://www.gnu.org/licenses/gpl-3.0)

Heavily based on the awesome ForcasWatch2 watchface by Matt Rossman https://github.com/mattrossman/forecaswatch2 , thank you very much for your great work and releasing it under open source!

This watchface adds in addition a weekly forecast, and a connection to [RiDuck](https://riduck.com/) to get current training advice.

## Screenshots

<div>
    <img src="screenshot/composite.png" alt="Color screenshot" style="display:inline-block;vertical-align: middle;">
</div>

## Features

* Current time
* Battery indicator
* 1 week calendar starting from current day
* 1 week weather forecast with max temperature and colored blue if rain is expected
* Connects to [RiDuck](https://riduck.com/) and shows traiing advice if login credentials are given in settings
* 24 hour weather forecast (updates every 30 minutes)
* Bluetooth connection indicator
* Quiet time indicator
* Weather provider OpenWeatherMap
* Current temperature
* Temperature forecast (red line)
* Precipitation probability forecast (blue area)
* City where forecast was fetched
* Next sunrise or sunset time
* GPS or manual location entry
* Fahrenheit and Celsius temperatures
* Customize time font and color
* Customize colors for Sundays, Saturdays, and US federal holidays
* Offline configuration page

## Platforms

Only Time and Time Steel are supported.

## Developers

### Building

Install and use the rebbletool found under https://github.com/richinfante/rebbletool

Build with
```
rebble build
```

Run with
```
rebble install --emulator basalt
```

Open app config with
```
rebble emu-app-config
```

If app config does not work, you can create the file src/pkjs/dev-config.js to set values for Clay keys (for convenience), e.g.
```
var owmApiKey = 'abc123';
module.exports.owmApiKey = owmApiKey;
```

If emulator does not go past the pebble screen kill emulator and do a 
```
rebble wipe
```

The `.pbw` output can be found in the `build` directory on the host machine.

