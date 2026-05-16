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

This builds the project with the Pebble SDK version pinned in `pebble-sdk-version` and provisioned by the repo scripts. The `.pbw` output can be found in the `build` directory.

## Supabase (telemetry)

Telemetry uses Supabase Edge Functions + Postgres.

Install/update the CLI through mise's aqua backend (already pinned in `mise.toml`):

```bash
mise install
supabase --version
```

Create a local telemetry hash secret (`TELEMETRY_HASH_SECRET`):

```bash
openssl rand -hex 32
```

Use this value for `TELEMETRY_HASH_SECRET`.

### Local stack and emulator testing

Start the local Supabase stack from the repo root:

```bash
supabase start
```

Copy `.env.example` to `.env`, then populate the telemetry keys there:

```bash
cp .env.example .env
```

Serve the telemetry edge function locally (from repo root):

```bash
mise telemetry-serve
```

For emulator validation, run:

```bash
mise install-emulator --logs
```

To verify inserts locally, open Supabase Studio at `http://127.0.0.1:54323` and inspect `public.telemetry_weather_fetch`.

### Hosted deployment (Supabase cloud)

Authenticate and link the repo to your Supabase project:

```bash
supabase login
supabase link --project-ref <your-project-ref>
```

Set function secrets in the hosted project:

```bash
supabase secrets set \
  TELEMETRY_HASH_SECRET=<paste-openssl-value>
```

Apply database migrations:

```bash
supabase db push --dry-run
supabase db push
```

Deploy the telemetry function:

```bash
supabase functions deploy telemetry-ingest
```

Wire release and preview builds to hosted telemetry by setting repository secret(s) used by CI workflows:

- `TELEMETRY_ENDPOINT=https://<your-project-ref>.supabase.co/functions/v1/telemetry-ingest`

### Optional: Supabase GitHub integration

If you use Supabase GitHub sync/branching, Supabase can auto-apply migrations and deploy functions from the `supabase/` directory when branches/PRs update. Enable it in Supabase Dashboard > Project Settings > Integrations.

`package.json` is generated from `package.template.json` and profile data in `profiles/`.

- Release profile: `profiles/package.release.json`
- Dev profile: `profiles/package.dev.json`

`mise build` and `mise build release` automatically generate `package.json` from the template/profile before building.

If you want the extra Pebble heap debug logs, set `ENABLE_MEMORY_LOGGING=1` in your `.env` before building or installing. This is independent of the dev/release package profile.

For deterministic emulator UI, set `FIXTURE=<name>` in `.env` before building or installing. Fixture files live in `fixtures/<name>.json` and define the watch facts and weather payload used by local builds.

Release notification copy (optional “what’s new” toast on upgrade) lives in `release-notifications.json`, keyed by the exact `version` string from the template (e.g. `"1.26.0"`); use `dev-config.js` `maxNotifiedVersion` to simulate skipped-version upgrades locally.

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

# Stop running emulator and phone simulator
mise kill-emulator

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
Local dev config has three layers:

- Use `.env` to choose the local mode or scenario, such as `FIXTURE=readme`.
- Use `fixtures/*.json` for committed, deterministic UI state: watch facts, Clay render settings, weather payloads, and other data that should make emulator screenshots reproducible.
- Use `src/pkjs/dev-config.js` for uncommitted behavior testing, including preloaded Clay settings when you are exercising real app flows instead of deterministic fixture UI.

When a fixture is active, prefer `claySettings` in the fixture for render-affecting Clay values. `dev-config.js` remains useful for local-only behavior switches and non-fixture testing.

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

Use this key in `src/pkjs/dev-config.js` to always show the notification for a **specific version key** from `release-notifications.json` on every app boot (ignores upgrade gating):

- `forceShowReleaseNotificationOnBoot = '1.26.0'` (string must match a key in `release-notifications.json` exactly)

Example:

```javascript
module.exports.forceShowReleaseNotificationOnBoot = '1.26.0';
```

Notes:

- Useful when `package.json` is still on an older version but you want to iterate on copy for the next release entry.
- Remove the key (or comment it out) when testing normal upgrade behavior.
- This is local-only dev behavior and is not written into Clay settings.

### Fixtures (emulator/dev)

Set `FIXTURE=<name>` in `.env` to load deterministic app-state data from `fixtures/<name>.json`.
Fixtures currently support:

- `watch.now`: local date/time fields used for C-rendered time/date UI.
- `watch.battery.percent`: battery level used for C-rendered battery UI, 0-100.
- `watch.battery.charging`: optional battery charging/plugged state, `true` or `false`.
- `watchSettings.timeFormat`: watch-level time display preference, `"12h"` or `"24h"`.
- `claySettings`: Clay-compatible settings keyed by `messageKey`, such as `"axisTimeFormat": "12h"`. Color settings use Pebble SDK color constants like `"GColorFolly"` from the Rebble color definitions: https://developer.rebble.io/docs/c/Graphics/Graphics_Types/Color_Definitions/
- `weather.city`: weather status city label.
- `weather.currentTemp`: current temperature in Fahrenheit.
- `weather.startHour`: local hour for the first forecast entry; fixture prep converts it to the runtime forecast timestamp.
- `weather.temps`: hourly Fahrenheit forecast values.
- `weather.precipPct`: hourly precipitation percentages, 0-100.
- `weather.sunEvents`: the next two sun events as local fields, e.g. `{ "type": "sunset", "dayOffset": 0, "hour": 20, "minute": 10 }`.

Minimal `.env`:

```bash
FIXTURE=readme
```

Fixture data is tracked in git inside `fixtures/`.

### Fixture emulator installs

Fixture time and battery state are compiled into the app through `watch.now` and `watch.battery`; `scripts/install-emulator.sh` does not call Pebble's emulator setting controls.

```bash
mise install-emulator --logs
```

## Upgrading pebble-tool

This project pins `pipx:pebble-tool` to an exact version in `mise.toml` (fully resolved in `mise.lock`).

To bump the pinned version:

```bash
mise upgrade "pipx:pebble-tool" --bump
mise install
```
