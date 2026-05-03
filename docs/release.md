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

Releases are ad-hoc signed by SEDER Productions inside the build pipeline (`codesign --sign -` on macOS, `New-SelfSignedCertificate` + signtool on Windows). Apple notarization requires a paid Apple Developer Program membership, and a non-warning Windows Authenticode signature requires a purchased code-signing certificate. Both are optional paid future steps and are not required for the free GitHub release pipeline.

## Manual Verification

After release assets are uploaded, download each one from GitHub Releases and confirm:

- the app launches, or the operating system shows the expected ad-hoc-signed warning (right-click → Open on macOS, "More info → Run anyway" on Windows SmartScreen)
- missing FFmpeg tools are reported clearly
- a valid source video can be probed
- TXT/CSV report export works
- export failure does not attempt re-encoding
