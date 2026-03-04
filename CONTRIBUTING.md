# Contributing

## Building

Prerequisites: [Node.js](https://nodejs.org/en/) and [mise](https://mise.jdx.dev/)

```bash
# Install toolchain from mise.toml
mise install

# Prepare release package.json from template
mise prepare-package

# Install JS dependencies
npm install

# Build
mise build
```

This builds the project with the Pebble SDK provisioned by mise. The `.pbw` output can be found in the `build` directory.

`package.json` is generated from `package.template.json` and profile data in `profiles/`.

- Release profile: `profiles/package.release.json`
- Dev profile: `profiles/package.dev.json`

You usually do not need to run prepare commands manually; `mise build` and `mise build-dev` run the correct prepare step automatically.

If you do want to regenerate explicitly:

```bash
mise prepare-package
mise prepare-package-dev
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

# Install dev build to basalt emulator (default profile)
mise install-emulator

# Explicit release installs when needed
mise install-phone-release <PHONE_IP>
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
