## Dev Iteration Flow

After you've added a feature run `mise build` to verify it builds.

If you need runtime logs, `mise install-emulator --logs` runs it in an emulator and prints logs to the terminal. The process stays alive until the emulator is closed

## Debugging

- C: `APP_LOG(APP_LOG_LEVEL_DEBUG, "msg", args)`
- Heap probes: use `MEMORY_LOG_HEAP("tag")` for dev-only `MEM|...` logs around lifecycle and redraw checkpoints.
- JS: `console.log("msg")`

## Message Keys

When adding AppMessage keys, update **both** `package.json` and `package.template.json` — they must stay in sync. After changing either file, run `pebble clean && pebble build` (not just `pebble build`) to force regeneration of the message keys header.

## Pebble Memory Tips

- Lazy-load bitmaps and destroy them when they are not needed to keep startup and steady-state heap usage low.
- Prefer drawing directly in an update proc over creating extra layer objects when a simple render path is enough.
- If a UI element only exists to paint pixels, keep it as light as possible instead of modeling it as a full layer.

## JavaScript Conventions

- For new JavaScript functions, add brief JSDoc (`@param`/`@returns`) annotations since this project does not use TypeScript.
- Prefer `Boolean(value)` over `!!value` in new/edited code for readability.

## Supabase migrations

Never write `migrations/` files manually. Edit declarative `schemas/` and generate migrations as-needed before commits with `supabase db diff -f <label>`
