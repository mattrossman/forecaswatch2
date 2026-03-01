# Contributing

## Building

Prerequisites: [Node.js](https://nodejs.org/en/) and [mise](https://mise.jdx.dev/)

```bash
# Install toolchain from mise.toml
mise install

# Install JS dependencies
npm install

# Build
mise build
```

This builds the project with the Pebble SDK provisioned by mise. The `.pbw` output can be found in the `build` directory.

You can run Pebble CLI commands directly, or use install tasks that build and install in one command:

```bash
# Option 1: set once in .env
cp .env.example .env
# then edit .env and set IP=<PHONE_IP>
mise install-phone

# Option 2: pass IP explicitly
mise install-phone <PHONE_IP>

# Install to basalt emulator
mise install-emulator
```

## Config
You can create a file `src/pkjs/dev-config.js` to set values for Clay keys (for convenience), e.g.

```javascript
var owmApiKey = 'abc123';
module.exports.owmApiKey = owmApiKey;
```

## Upgrading pebble-tool

This project pins `pipx:pebble-tool` to an exact version in `mise.toml` so the version is visible at a glance.

If you need to reset the pin to the current latest once:

```bash
mise use "pipx:pebble-tool" --pin
```

When a new release is available, bump the pinned version and refresh installs:

```bash
mise upgrade "pipx:pebble-tool" --bump
mise install
```

Optional check before upgrading:

```bash
mise upgrade "pipx:pebble-tool" --dry-run
```
