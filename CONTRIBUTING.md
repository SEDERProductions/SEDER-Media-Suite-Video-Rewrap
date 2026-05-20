# Contributing

Thanks for helping improve Video Rewrap.

## Development

Required tools:

- Rust stable
- Node.js 20 or newer
- Qt 6.5 or newer (Core, Gui, Network, Qml, Quick, QuickControls2, Widgets, Test)
- CMake 3.24 or newer
- FFmpeg, FFprobe, and FFplay for manual media testing

Run checks before opening a pull request:

```sh
cargo fmt --check
cargo clippy --all-targets -- -D warnings
cargo test
cmake -S qt -B qt/build/release -DCMAKE_BUILD_TYPE=Release
cmake --build qt/build/release
ctest --test-dir qt/build/release
```

## Branch Policy

- Develop on feature branches; open a pull request against `main`.
- One concern per pull request — undo/redo, recent files, etc. each belong
  in their own change so review is tractable.
- Keep commits focused; commit messages should explain *why*, not *what*.
- Avoid committing build output, generated packages, local screenshots, or
  machine-specific files.

## Architecture Notes

- The Rust crate (`src/`) is the source of truth for media logic — every
  FFmpeg / ffprobe command is constructed there, never built in C++.
- The Qt layer (`qt/src/`) handles OS integration: file dialogs, process
  execution, QSettings persistence, network calls. Heavy work runs on
  worker threads; nothing blocking must run on the QML render thread.
- QML (`qt/qml/Main.qml`) is presentation only; route actions through
  `app.*` Q_INVOKABLE methods on `AppController`.
- Wrap every user-visible string in `qsTr(...)` so translations stay possible.

## Tests

- Rust: every helper in `media_core` has at least one test in
  `video_rewrap_tests.rs`. New helpers must include parser / round-trip
  coverage.
- Qt: `qt/tests/AppControllerTests.cpp` and
  `qt/tests/SegmentTableModelTests.cpp` host unit tests; add new test
  classes via `qt_add_executable` + `add_test` in `qt/CMakeLists.txt`.
- Avoid tests that depend on FFmpeg being installed. Stub via the
  testing hooks exposed on each class.

## Release

See [`docs/release.md`](docs/release.md). Tagging `v<MAJOR>.<MINOR>.<PATCH>`
triggers the release workflow, which also publishes a `latest.json` for the
in-app update checker.
