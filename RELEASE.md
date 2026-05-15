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