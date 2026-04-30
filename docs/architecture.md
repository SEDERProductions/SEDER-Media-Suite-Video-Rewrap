# Architecture

Video Rewrap has a Qt 6/QML interface with a compiled Rust media core.

## Rust Core

`src/media_core/video_rewrap.rs` owns the business logic:

- timecode parsing and formatting
- FFprobe metadata/keyframe parsing
- nearest-keyframe snapping
- segment validation and duration calculation
- FFmpeg/FFprobe/FFplay command construction
- project JSON and report generation

`src/ffi.rs` exposes that logic to C++ through a small C ABI. Every exported function returns a JSON string with an `ok` boolean and either result fields or an `error` field. The caller releases strings with `svr_free_string`.

## Qt Backend

`qt/src/` owns desktop behavior:

- native file dialogs
- process execution through `QProcess`
- background probe/export work on `QThread`
- status/progress updates through Qt signals
- `SegmentTableModel`, a `QAbstractTableModel` used by QML `TableView`

Heavy work must not run on the QML/UI thread.

## QML Interface

`qt/qml/Main.qml` defines the operational app layout:

- compact header
- left navigation/status rail
- dense inspector-style controls
- metadata and keyframe workspace
- virtualized segment table
- persistent footer log/status area

The app stays local-first and does not upload media.
