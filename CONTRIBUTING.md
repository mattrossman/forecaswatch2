# Contributing

## Building

Prerequisites: [Node.js](https://nodejs.org/en/) and [mise](https://mise.jdx.dev/)

```bash
# Install toolchain from mise.toml
mise install

# Install JS dependencies
npm install

# Build dev (default)
mise build

# Build release
mise build release
```

This builds the project with the Pebble SDK provisioned by mise. The `.pbw` output can be found in the `build` directory.

`package.json` is generated from `package.template.json` and profile data in `profiles/`.

- Release profile: `profiles/package.release.json`
- Dev profile: `profiles/package.dev.json`

`mise build` and `mise build release` automatically generate `package.json` from the template/profile before building.

If you want to regenerate `package.json` without building:

```bash
mise prepare-package           # dev profile (default)
mise prepare-package release   # release profile
```

You can run Pebble CLI commands directly, or use install tasks that build and install in one command:

```bash
# Option 1: set once in .env
cp .env.example .env
# then edit .env and set IP=<PHONE_IP>
# this installs the dev build by default
mise install-phone

# Option 2: pass IP explicitly
mise install-phone <PHONE_IP>

# Explicit release install
mise install-phone <PHONE_IP> release

# Pass through pebble install flags
mise install-phone --logs

# Legacy pass-through separator (still works)
mise install-phone -- --logs

# Install dev build via CloudPebble (default profile)
mise install-cloud

# Explicit release install via CloudPebble
mise install-cloud release

# Install dev build to emulator (defaults: profile=dev, emulator=basalt)
mise install-emulator

# Choose emulator platform
mise install-emulator aplite

# Choose emulator build type
mise install-emulator release

# Choose emulator platform and build type
mise install-emulator release aplite

# Pass through pebble install flags
mise install-emulator --logs

# Set default emulator in environment
PEBBLE_EMULATOR=aplite mise install-emulator

# Legacy pass-through separator (still works)
mise install-emulator -- --logs

# Take a screenshot from emulator (default platform: basalt)
mise screenshot-emulator

# Choose emulator platform
mise screenshot-emulator aplite

# Legacy flag form (still works)
mise screenshot-emulator -- --emulator chalk

# Set default emulator in environment
PEBBLE_EMULATOR=aplite mise screenshot-emulator

# Default output goes to screenshot/tmp/<timestamp>-<platform>.png

# Provide explicit output path / additional screenshot args
mise screenshot-emulator -- screenshot/my-capture.png --no-open --no-correction

# Take a screenshot from phone
mise screenshot-phone

# Or pass IP explicitly
mise screenshot-phone <PHONE_IP>

# Default output goes to screenshot/tmp/<timestamp>.png

# Provide explicit output path / additional screenshot args
mise screenshot-phone -- screenshot/my-capture.png
```

## Config
You can create `src/pkjs/dev-config.js` to override Clay keys and local dev behavior.

Example:

```javascript
module.exports.owmApiKey = 'abc123';
```

### PKJS storage reset (dev)

Use this key in `src/pkjs/dev-config.js` to force a PKJS `localStorage` reset on each app boot while enabled:

- `clearPkjsStorageOnBoot = true`

Example:

```javascript
module.exports.clearPkjsStorageOnBoot = true;
```

Notes:

- Keep this set to `true` only while testing first-install behavior.
- Set it back to `false` before testing upgrade-notification behavior.
- This is local-only dev behavior and is not written into Clay settings.

### Release notification preview (dev)

Use this key in `src/pkjs/dev-config.js` to always show the configured release notification on app boot:

- `forceShowReleaseNotificationOnBoot = true`

Example:

```javascript
module.exports.forceShowReleaseNotificationOnBoot = true;
```

Notes:

- This bypasses upgrade gating only for showing the notification (helpful for copy/visual iteration).
- Set it back to `false` (or remove it) when testing normal upgrade behavior.
- This is local-only dev behavior and is not written into Clay settings.

### Mock weather (emulator/dev)

Use these keys in `src/pkjs/dev-config.js`:

- `provider = 'mock'` enables the mock provider.
- `mockCity` sets the city label independently.
- `mockScenario` selects the active built-in scenario.

Scenario data is tracked in git inside `src/pkjs/weather/mock.js` (`MOCK_SCENARIOS`).

Minimal shape:

```javascript
module.exports.provider = 'mock';
module.exports.mockCity = 'New York, NY';
module.exports.mockScenario = 'clearMorning';
```

Notes:

- If `mockScenario` is missing/invalid, the app falls back to the first built-in scenario.
- To add/edit scenarios, update `MOCK_SCENARIOS` in `src/pkjs/weather/mock.js`.
- `startEpoch` and `sunEvents[].epoch` should be coherent in emulator local time (the graph and shading use watch localtime).

### Emulator time overrides (applied automatically)

`scripts/install-emulator.sh` reads these keys from `dev-config.js` after install:

- `emuTimeFormat`: `12h` or `24h`
- `emuTime`: `HH:MM:SS` or Unix seconds (e.g. `1772870400`)

Then run:

```bash
mise install-emulator --logs
```

Reset behavior when keys are removed:

- `emuTimeFormat` defaults to `24h`
- `emuTime` fallback order is:
  1. explicit `emuTime` in `dev-config.js`
  2. `emuTime` of active mock scenario (when `provider = 'mock'`, if present)
  3. `startEpoch` of active mock scenario (when `provider = 'mock'`)
  4. current host time

## Upgrading pebble-tool

This project pins `pipx:pebble-tool` to an exact version in `mise.toml` (fully resolved in `mise.lock`).

To bump the pinned version:

```bash
mise upgrade "pipx:pebble-tool" --bump
mise install
```
