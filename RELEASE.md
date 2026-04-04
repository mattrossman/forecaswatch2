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