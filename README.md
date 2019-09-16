
# ForecasWatch 2
[![GitHub release (latest by date)](https://img.shields.io/github/v/release/mattrossman/forecaswatch2?label=Download&color=85C1E9 )](https://github.com/mattrossman/forecaswatch2/releases/latest/download/forecaswatch2.pbw)
[![Build status](https://img.shields.io/travis/mattrossman/forecaswatch2/master?label=Travis%20CI)](https://travis-ci.org/mattrossman/forecaswatch2)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)


Once upon a time I relied on *ForecasWatch* as the daily driver watchface on my beloved red Pebble Time. Recently, the free tier of the Weather Underground API on which the watchface relied was discontinued, making a huge portion of the watchface unusable.

The developer, RCY, is nowhere to be found in the Rebble era. I plan to continue using my Pebble(s) for years to come, so this is my attempt to revive this wonderful watchface—and this time it's open source!

## Progress

![screenshot](https://i.imgur.com/EA8vTt9.png)

* Current time
* Color-coded battery indicator
* 3 week calendar
* 24 hour weather forecast
* Multiple weather providers (Weather Underground*, DarkSky)
* Current temperature
* Temperature forecast (red line)
* Precipitation probability forecast (blue area)
* City where forecast was fetched
* Next sunrise or sunset time
* Updates every 30 minutes

*\* Using a hacky workaround*

## Installation

Download the latest [`forecaswatch2.pbw`](https://github.com/mattrossman/forecaswatch2/releases/latest/download/forecaswatch2.pbw) release. On Android you can use [Amaze File Manager](https://play.google.com/store/apps/details?id=com.amaze.filemanager&hl=en_US) to open this file through the Pebble app.

## Developers

### Building

While I use the SDK natively for development, I suggest using the Pebble SDK Docker image hosted [here](https://hub.docker.com/r/dmorgan81/rebble) to avoid the headache of installation. I use this image in my Travis CI builds.

### Config
You can create a file `src/pkjs/dev-config.js` to set values for Clay keys (for convenience), e.g.

```javascript
var darkSkyApiKey = 'abc123';
module.exports.darkSkyApiKey = darkSkyApiKey;
```