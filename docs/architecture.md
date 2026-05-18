# Architecture

Video Rewrap has a Qt 6/QML interface with a compiled Rust media core.

## Why this split

The codebase deliberately runs two languages across one boundary. Rust owns media semantics — timecode parsing, keyframe snapping, segment duration math, FFmpeg argument construction, and report generation. Qt owns desktop semantics — native file dialogs, QML rendering, `QProcess` execution, and signal/slot threading. The boundary is `src/ffi.rs` (a small C ABI returning JSON) marshalled into C++ by `qt/src/RustBridge`.

**Why Rust for the core, not C++.** Timecode formats, segment validation, and FFmpeg command construction have a large surface of cases. Exhaustive pattern matching catches missed cases at compile time, and ownership rules prevent the off-by-one and aliasing bugs that segment-boundary math is prone to. The command-construction logic (offsets, stream mapping, codec preservation) is centralised in one type-safe place so invalid `ffmpeg` invocations cannot escape into `QProcess`. Pure functions returning JSON are unit-testable without a UI.

**Why Qt for the shell, not a Rust GUI toolkit.** Native file dialogs and platform conventions on macOS, Windows, and Linux come for free. `QThread` and signals/slots handle the async probe/export work without bespoke plumbing. QML fits the dense inspector-style layout in `qt/qml/Main.qml`. Rust GUI toolkits (egui, iced, slint) were not production-ready for native desktop integration when this project started, and the gap on platform polish remains real.

**Why not collapse to one language.** All-Rust loses the native dialog and threading maturity and gains little, since the media logic is already in Rust. All-C++/Qt requires reimplementing validation, timecode, and command-construction logic without Rust's compile-time guarantees, raising the long-term bug surface. Neither alternative pays for itself.

**Discipline that keeps the split honest.** Rust *constructs* FFmpeg/FFprobe/FFplay argument lists; Qt only *executes* them via `QProcess`, so injection-safety and offset math live in exactly one place. The FFI is one-directional — Qt calls Rust and reads JSON back; Rust never holds Qt state. No logic is duplicated across the boundary.

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
