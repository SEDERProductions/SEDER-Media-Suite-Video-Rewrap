# SEDER Video Rewrap

SEDER Video Rewrap is a local-first desktop utility for building keyframe-aligned video segments and exporting them with FFmpeg stream copy. It is designed for post-production workflows where rewrapping should avoid re-encoding.

[Download the latest release](https://github.com/SEDERProductions/SEDER-Media-Suite-Video-Rewrap/tags)

Builds are ad-hoc signed by SEDER Productions for tamper-detection. macOS may still ask you to right-click → Open the first time, and Windows SmartScreen will show a "More info → Run anyway" prompt — Apple notarization and a paid Windows Authenticode certificate are not configured. Linux builds ship as an AppImage when the release runner can produce one, otherwise as a ZIP.

## Features

- Qt 6/QML desktop interface.
- Compiled Rust media core for timecode, keyframe, segment, project, and report logic.
- FFprobe metadata and keyframe discovery.
- Embedded video preview with an interactive keyframe timeline, draggable playhead, IN/OUT range, segment preview, and IN/OUT frame thumbnails.
- External FFplay preview as a fallback for codecs the in-app player cannot decode.
- Drag-and-drop to open, keyboard shortcuts, and an overwrite/identity guard before export.
- FFmpeg stream-copy export with no re-encode fallback.
- Project save/load plus TXT and CSV report export.

All processing is local. The app does not upload media files.

## Requirements

Video Rewrap does not bundle FFmpeg tools. Install them separately:

- macOS: `brew install ffmpeg`
- Windows: install FFmpeg from `ffmpeg.org` and add `ffmpeg`, `ffprobe`, and `ffplay` to `PATH`
- Linux: install FFmpeg from your package manager, including `ffplay`

For development:

- Rust stable
- Node.js 20 or newer
- Qt 6.5 or newer, including the Qt Multimedia module (for the in-app video preview)
- CMake 3.24 or newer

## Build

Rust checks:

```sh
cargo fmt --check
cargo check
cargo test
```

Qt build:

```sh
cmake -S qt -B qt/build/release -DCMAKE_BUILD_TYPE=Release
cmake --build qt/build/release --config Release
ctest --test-dir qt/build/release --build-config Release
```

macOS with Homebrew:

```sh
brew install qt cmake ninja
export PATH="$(brew --prefix qt)/bin:$PATH"
export QT_PREFIX="$(brew --prefix qt)"
cmake -S qt -B qt/build/release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$QT_PREFIX"
cmake --build qt/build/release
ctest --test-dir qt/build/release
```

Run the app from the build output:

```sh
./qt/build/release/bin/seder-video-rewrap
```

## Package

Create a local release package for the current platform:

```sh
npm install
npm run package:release
```

Release files are written to `release/`.

## GitHub Releases

Pushing a tag such as `v0.1.0` triggers GitHub Actions to build macOS, Windows, and Linux release assets using standard GitHub-hosted public-repo runners. The generated artifacts and SHA-256 checksum files are uploaded to the matching GitHub Release.

GitHub Actions and GitHub Releases are expected to be free for this public repository when using standard runners. Builds are ad-hoc signed by SEDER Productions; Apple notarization (paid Developer Program) and Windows Authenticode (paid certificate) are optional future steps.

## License

Code is licensed under GPL-3.0-only. See [LICENSE](LICENSE).

The SEDER Productions name, logo, and related branding are trademarks or brand identifiers of SEDER Productions. The GPL license applies to the code, but it does not grant permission to use SEDER Productions branding for unrelated redistribution.
