# Changelog

## 1.0.0

First stable release of SEDER Video Rewrap. Highlights since 0.1.x:

### Added
- Undo / redo for every segment edit (Ctrl+Z / Ctrl+Shift+Z), cleared on
  project load so cross-project undo can't corrupt state.
- Recent files: 10 most recent source videos and projects appear in a
  File menu submenu and persist between launches.
- Drag-and-drop a `.mov` / `.mp4` / `.mkv` or `.seder-rewrap.json` onto
  the window to open it.
- Keyboard shortcuts: Ctrl+O (open video), Ctrl+Shift+O (load project),
  Ctrl+S (save project), Ctrl+E (start export), Ctrl+Z / Ctrl+Shift+Z
  (undo / redo), Delete (remove selected segment), Space (toggle
  enabled), arrow keys (row navigation), I / O (in / out markers),
  comma / period (step keyframes).
- About dialog with version, license, and update opt-in.
- Auto-update channel: the app fetches a `latest.json` from GitHub
  Releases on launch and surfaces a non-blocking banner when a newer
  release is available. Honours the persisted "Check on launch"
  preference and the `--no-update-check` CLI flag.
- FFmpeg version gating: exports refuse to start on FFmpeg < 4.0 and
  display the detected version next to the recheck button.
- FFmpeg path override: Tools → FFmpeg Path… picks a folder containing
  ffmpeg / ffprobe / ffplay and prepends it to PATH (persisted).
- Export error dialog with a "Copy Log" button that puts the full
  ffmpeg stderr tail, exit code, and segment index on the clipboard.
- Project format migration: pre-1.0 project files (no `version` field)
  load and re-save at the current schema; files saved by a newer build
  are rejected with a clear "update the app" error.
- Accessibility labels on all interactive controls; full menu bar
  (File / Edit / Tools / Help).
- i18n scaffolding: every user-visible string flows through `qsTr()`.

### Changed
- ExportEngine emits a structured `errorReport` signal alongside the
  user-facing log line so the QML layer can show a rich error dialog.
- ProcessUtil exposes `programVersionOutput()` for the new version
  gate.
- Build wires Qt6::Network for the update checker and threads the
  CMake project version through `SEDER_APP_VERSION`.

## 0.1.0

- Initial open-source Video Rewrap release.
- Qt 6/QML desktop interface backed by a compiled Rust media core.
- Keyframe-aligned FFmpeg stream-copy segment export.
- Project save/load and TXT/CSV report export.
