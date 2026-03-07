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
You can create a file `src/pkjs/dev-config.js` to set values for Clay keys (for convenience), e.g.

```javascript
var owmApiKey = 'abc123';
module.exports.owmApiKey = owmApiKey;
```

## Upgrading pebble-tool

This project pins `pipx:pebble-tool` to an exact version in `mise.toml` (fully resolved in `mise.lock`).

To bump the pinned version:

```bash
mise upgrade "pipx:pebble-tool" --bump
mise install
```
