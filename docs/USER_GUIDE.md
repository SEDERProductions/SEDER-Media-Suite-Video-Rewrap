# SEDER Video Rewrap — User Guide

A short, practical walkthrough. For build / contribute notes see
[`CONTRIBUTING.md`](../CONTRIBUTING.md); for architecture see
[`architecture.md`](architecture.md).

## Installation

Download a release from the
[GitHub Releases](https://github.com/SEDERProductions/SEDER-Media-Suite-Video-Rewrap/releases)
page.

- **macOS**: the `.app` is ad-hoc signed; on first launch right-click → **Open**
  and confirm the Gatekeeper prompt.
- **Windows**: SmartScreen will show a "Windows protected your PC" dialog —
  click **More info → Run anyway**. Paid Authenticode signing is not enabled.
- **Linux**: unzip and run the executable, or use the `.AppImage` when provided.
  The bundled `.desktop` file registers the `application/x-seder-rewrap-project`
  MIME type for project files.

The app does not bundle FFmpeg. Install it separately:

- macOS: `brew install ffmpeg`
- Windows: download from [ffmpeg.org](https://ffmpeg.org/) and add `ffmpeg`,
  `ffprobe`, `ffplay` to `PATH`
- Linux: install via your package manager (`apt install ffmpeg`, etc.)

If FFmpeg cannot be added to `PATH`, use **Tools → FFmpeg Path…** to point
the app at the folder that contains the binaries.

## First Run

1. **Open Video** (Ctrl+O) or drag a `.mov` / `.mp4` / `.mkv` onto the window.
2. The app probes the file with `ffprobe` and lists detected keyframes.
3. Use **,** / **.** (or the Previous / Next keyframe buttons) to walk through
   keyframes. Press **I** to set an IN marker, **O** for the OUT marker.
4. Type a name (and notes) and click **Create Segment** — the segment is
   appended to the list at the bottom.
5. Pick an **Output File** (any of `.mov`, `.mp4`, `.mkv`).
6. **Start Export** (Ctrl+E) runs `ffmpeg -c copy` for each segment and either
   concatenates them into a single output (default) or writes them to separate
   files based on the export mode toggle.

## Working with Segments

- Click a row to select. Use ↑ / ↓ to move between rows.
- **Space** toggles the selected row enabled / disabled.
- **Delete** removes it.
- **Move Up / Move Down** reorder; **Duplicate** clones a row.
- Every change is undoable with **Ctrl+Z** and re-doable with **Ctrl+Shift+Z**.

## Projects

- **Save Project** (Ctrl+S) writes a `.seder-rewrap.json` with the source path,
  output path, and segments.
- **Load Project** (Ctrl+Shift+O) restores them. File menu → Recent Projects
  shows the 10 most recent.
- Project files include a schema version; older files migrate silently to the
  current schema on load. Files saved by a newer build than the one you are
  running are rejected with an "update the app" message.

## Replace File

The **Replace File** button renames the source video to `<name>_PREWRAP.<ext>`,
then exports a new file at the original path. Use this when you want to
non-destructively swap the working file in a directory without changing the
filename used by other tools (DaVinci Resolve, etc.).

## Updates

On launch the app checks `latest.json` on the release page and surfaces a
non-blocking banner when a newer version is available. Click **Download** to
open the release page in your system browser. Disable the check from
**Help → About → Check for updates on launch**, or pass `--no-update-check`
on the command line for offline / CI builds.

## Keyboard Shortcuts

| Action                       | Shortcut             |
| ---------------------------- | -------------------- |
| Open video                   | Ctrl+O               |
| Load project                 | Ctrl+Shift+O         |
| Save project                 | Ctrl+S               |
| Start export                 | Ctrl+E               |
| Undo / redo                  | Ctrl+Z / Ctrl+Shift+Z|
| Remove selected segment      | Delete               |
| Toggle selected segment      | Space                |
| Move row up / down           | ↑ / ↓                |
| Set IN / OUT marker          | I / O                |
| Previous / next keyframe     | , / .                |

## Troubleshooting

**"FFmpeg and/or FFprobe are missing."**  Install FFmpeg, or use
**Tools → FFmpeg Path…** to point at a folder containing the binaries.

**"Detected FFmpeg X.Y.Z which is older than the required 4.0.0."**  Stream-copy
export needs ≥ 4.0. Update FFmpeg.

**"Segment 'X' in point is not a detected keyframe."**  The in/out markers must
land on real keyframes. Use the previous/next keyframe buttons to snap.

**Export error dialog with a long stderr log.**  Click **Copy Log** and include
the text when filing an issue.

**No update banner appears even though a new release exists.**  Network access
may be blocked, or the launch check is disabled — open
**Help → About** and tick "Check for updates on launch", then click **Check Now**.
