## Automatic GitHub releases (Release Please)

Releases are managed by `.github/workflows/release-please.yml`.

Flow:

1. Merge conventional commits into `main`.
2. Release Please opens or updates a release PR with version bumps.
3. Merge the release PR.
4. The workflow creates a GitHub release and uploads `build/forecaswatch2.pbw`.

## Developer portals

- [Rebble Developer Portal](https://dev-portal.rebble.io/
)
- [Pebble Developer Dashboard (RePebble)](https://developer.repebble.com/dashboard)

## Screenshots

Device frames and banner templates were originally obtained from [Appstore Assets](https://developer.rebble.io/guides/appstore-publishing/appstore-assets/) in Pebble docs.

Those download links are since broken (see https://github.com/pebble-dev/developer.rebble.io/issues/37), but you can still download from the wayback machine (e.g. [banner templates](https://web.archive.org/web/20161207160612/https://s3.amazonaws.com/developer.getpebble.com/assets/other/banner-templates-design.zip)).

Composite screenshots with:

```sh
mise composite <version>
mise composite-screenshot <pebble-time-red|pebble2-duo-white|pebble-time2-red> <screenshot.png> <output.png>
```

`mise composite <version>` reads `screenshot/<version>/raw/*.png` and writes the matching framed PNGs to `screenshot/<version>/composite/`.

Some downloaded frame assets used older names. Track them with the modern device names: `core-time2-red.svg` becomes `pebble-time2-red.svg`, and `pebble2-white.svg` becomes `pebble2-duo-white.svg`. The Pebble Time 2 rename is noted in Eric Migicovsky's July Pebble update: https://ericmigi.com/blog/july-pebble-update/
