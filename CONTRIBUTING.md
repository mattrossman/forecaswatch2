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

# Install dev build to basalt emulator (default profile)
mise install-emulator

# Explicit release install
mise install-emulator release

# Pass through pebble install flags
mise install-emulator -- --logs
mise run install-emulator dev -- --logs
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
