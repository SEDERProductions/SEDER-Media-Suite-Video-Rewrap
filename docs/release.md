# Release Process

GitHub Releases are the canonical download page.

## Automated Release

1. Update `CHANGELOG.md`.
2. Ensure version fields match the release version.
3. Push a tag:

```sh
git tag v0.1.0
git push origin v0.1.0
```

The `release.yml` workflow builds macOS, Windows, and Linux assets on standard GitHub-hosted runners and uploads them to the GitHub Release for that tag.

## Expected Assets

- `seder-video-rewrap-v0.1.0-macos-arm64.zip`
- `seder-video-rewrap-v0.1.0-windows-x64.zip`
- `seder-video-rewrap-v0.1.0-linux-x64.AppImage` or `seder-video-rewrap-v0.1.0-linux-x64.zip`
- matching `.sha256` files

## Signing

The first release ships unsigned. macOS notarization requires an Apple Developer Program membership, and Windows signing requires a code-signing certificate. These are optional paid steps and are not required for the free GitHub release pipeline.

## Manual Verification

After release assets are uploaded, download each one from GitHub Releases and confirm:

- the app launches, or the operating system shows the expected unsigned-app warning
- missing FFmpeg tools are reported clearly
- a valid source video can be probed
- TXT/CSV report export works
- export failure does not attempt re-encoding
