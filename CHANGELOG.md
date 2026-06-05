# Changelog

## Unreleased

- Embedded video preview built on Qt Multimedia: an interactive keyframe timeline with a draggable, keyframe-snapping playhead, IN/OUT range highlight, segment preview, and IN/OUT frame thumbnails.
- External FFplay preview retained as a fallback for codecs the in-app player cannot decode.
- Usability: drag-and-drop to open a video, a dismissible error banner, a probing indicator, keyboard shortcuts (arrows / I / O / Space), and an overwrite/identity guard before export.

## 0.1.0

- Initial open-source Video Rewrap release.
- Qt 6/QML desktop interface backed by a compiled Rust media core.
- Keyframe-aligned FFmpeg stream-copy segment export.
- Project save/load and TXT/CSV report export.
