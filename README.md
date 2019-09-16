
# ForecasWatch 2
[![GitHub release (latest by date)](https://img.shields.io/github/v/release/mattrossman/forecaswatch2?label=latest&color=85C1E9 )](https://github.com/mattrossman/forecaswatch2/releases/latest/download/forecaswatch2.pbw)
![GitHub All Releases](https://img.shields.io/github/downloads/mattrossman/forecaswatch2/total?label=downloads&logo=data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAMAAADXqc3KAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAM1BMVEUAAAD///////////////v///z///z///z///v///v///v///v///v///3///z///z///xB67/PAAAAEHRSTlMAERwgPExhYouMjY6P1N7g8FyKngAAAF5JREFUKM/VjksOgCAMBR+IH0CB+59WEBpTPsYts+tM2hTIiLUgwFGhoKYN+xVxFFyatlx0qDhox3Cv32tm4FnR/CtL3lTvUmk8cCZvgW7p+lgG/j9yaZBP8KHBf4QbpMkNa908ZS8AAAAASUVORK5CYII=)
[![Build status](https://img.shields.io/travis/mattrossman/forecaswatch2/master?label=build&logo=travis)](https://travis-ci.org/mattrossman/forecaswatch2)
[![GitHub](https://img.shields.io/github/license/mattrossman/forecaswatch2?logo=data%3Aimage%2Fpng%3Bbase64%2CiVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAACXBIWXMAAAsTAAALEwEAmpwYAAAB7ElEQVRIie2TPWsUURSGnxsWjLosIgTJLpL8AbWwUKwjgpg%2FYEAQxCiCSLKgsAppBG38AFNolU72P4hlrLZQEwsVxSD4AWmyKFmLPBY5wWGys9ktBAtfGJh73o9z7p078K9ATeqkOqWmv9Fg0j84269vaIAelcx7u19TaSeBOgocBYaBlxE%2Bop4BWimlL32PqJ5Sj6hVdVZtqRsWYyM0dbUW3tNF4eM9gt6py5n1ctSKMF7U5IL6NkRN9ZxaC24uEzAXtVponkb9vTrd64h2qSshvpzjtjXIcBej%2FlkdznL5WzQNjALPgUZeXDQUcCs8B4BLRcI96lf1cXyPjjqz0w7Ua6EdU59Ext5uDa6rP9VqrB%2Bq39VyUQO1rH5TH8S6Ghk38uEVdVW9k6mNqG210aNBQ11TRzK%2Bu5FVASjFywKwH9id%2B4AfgLr6usuOJ4E68BG4om5Rw5G1oJ4vAevAsSCvbju7TYwV1PbFc7gLfxxYH0op%2FQJOAktdRB3gNvCoCzcfXKcLtwRMpJQ6JYCU0hvgkHoipqkAn4BnKaVVgMwREB6Bm%2Bp9YCJ2tAa8SiktbulKOdMisMgAiAGag3i2QZ2JG7WFdvYf6TlAj9CDwBSbt2IWKOckbeAe8ANoppRWBp26Zf94MVD4f2TxGzyXVgXKldOBAAAAAElFTkSuQmCC)](https://www.gnu.org/licenses/gpl-3.0)


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

## Platforms

Currently only the Basalt platform (Pebble Time, Pebble Time Steel) is supported.

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