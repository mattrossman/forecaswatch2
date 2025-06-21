## Tagging
```bash
git tag -a v1.0.0 -m "v1.0.0"
```

```bash
git push --atomic origin master v1.0.0
```

## Publishing

Clear `build/` folder before running build to prevent stale version appearing on config page.

Watchface file is found at `build/velowatch.pbw`

To upload to Rebble store:
https://dev-portal.rebble.io/