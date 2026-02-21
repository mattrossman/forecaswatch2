## Automatic GitHub releases (Release Please)

Releases are managed by `.github/workflows/release-please.yml`.

Flow:

1. Merge conventional commits into `main`.
2. Release Please opens or updates a release PR with version bumps.
3. Merge the release PR.
4. The workflow creates a GitHub release and uploads `build/forecaswatch2.pbw`.

## Rebble store publishing

Download the generated release asset (`forecaswatch2.pbw`) and upload it to Rebble:
https://dev-portal.rebble.io/
