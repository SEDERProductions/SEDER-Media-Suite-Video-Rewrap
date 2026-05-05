use super::video_rewrap::*;
use std::path::Path;
use tempfile::tempdir;

const DEFAULT_KEYFRAMES: [i64; 5] = [0, 1_000, 2_500, 5_000, 8_000];
const DRIFTED_KEYFRAMES: [i64; 3] = [0, 1_000, 2_001];
const LEGACY_PROJECT_JSON: &str = r#"{
        "source_file": "src.mov",
        "output_file": "out.mov",
        "segments": [
            {"name": "A", "in_ms": 0, "out_ms": 1000, "notes": "", "enabled": true}
        ]
    }"#;

fn mk_keyframes(values: &[i64]) -> Vec<i64> {
    values.to_vec()
}

fn mk_segment(name: &str, in_ms: i64, out_ms: i64) -> RewrapSegment {
    RewrapSegment {
        name: name.into(),
        in_ms,
        out_ms,
        notes: String::new(),
        enabled: true,
    }
}

fn mk_segment_with_notes(name: &str, in_ms: i64, out_ms: i64, notes: &str) -> RewrapSegment {
    RewrapSegment {
        notes: notes.into(),
        ..mk_segment(name, in_ms, out_ms)
    }
}

#[test]
fn parses_ffprobe_keyframe_output() {
    let parsed = parse_ffprobe_keyframes("0.000000\n2.500000\nN/A\n1.000000\n2.500000\n");
    assert_eq!(parsed, vec![0, 1_000, 2_500]);
}

#[test]
fn parses_ffprobe_metadata() {
    let output = "format_name=mov,mp4,m4a,3gp,3g2,mj2\nduration=12.345000\ncodec_name=h264\nwidth=1920\nheight=1080\navg_frame_rate=24000/1001\n";
    let meta = parse_ffprobe_metadata(output, Path::new("/tmp/interview.mov"), 99);
    assert_eq!(meta.filename, "interview.mov");
    assert_eq!(meta.duration_ms, Some(12_345));
    assert_eq!(meta.width, Some(1920));
    assert_eq!(meta.height, Some(1080));
    assert_eq!(meta.codec.as_deref(), Some("h264"));
}

#[test]
fn parses_and_formats_timecode() {
    assert_eq!(parse_timecode_to_ms("00:02:10.250").unwrap(), 130_250);
    assert_eq!(parse_timecode_to_ms("3.5").unwrap(), 3_500);
    assert_eq!(format_ms(130_250), "00:02:10.250");
}

#[test]
fn snaps_to_nearest_keyframe() {
    let snap = nearest_keyframe(&mk_keyframes(&DEFAULT_KEYFRAMES), 2_100).unwrap();
    assert_eq!(snap.snapped_ms, 2_500);
    assert_eq!(snap.distance_ms, 400);
}

#[test]
fn calculates_segment_and_total_duration() {
    let segments = vec![
        mk_segment("A", 1_000, 2_500),
        RewrapSegment { enabled: false, ..mk_segment("B", 2_500, 8_000) },
        mk_segment("C", 5_000, 8_000),
    ];
    assert_eq!(segment_duration(&segments[0]), 1_500);
    assert_eq!(total_enabled_duration(&segments), 4_500);
}

#[test]
fn reorders_segments() {
    let mut segments = vec![
        mk_segment("A", 0, 1_000),
        mk_segment("B", 1_000, 2_500),
    ];
    move_segment(&mut segments, 1, -1);
    assert_eq!(segments[0].name, "B");
}

#[test]
fn validates_invalid_segments() {
    let bad = mk_segment("Bad", 1_200, 2_500);
    assert!(validate_segment(&bad, &mk_keyframes(&DEFAULT_KEYFRAMES)).is_err());
    let reversed = mk_segment("Reversed", 2_500, 1_000);
    assert!(validate_segment(&reversed, &mk_keyframes(&DEFAULT_KEYFRAMES)).is_err());
}

#[test]
fn generates_ffmpeg_commands_without_shell_strings() {
    let segment = mk_segment("A", 1_000, 2_500);
    let command = ffmpeg_segment_command(
        Path::new("/media/source.mov"),
        &segment,
        &mk_keyframes(&DEFAULT_KEYFRAMES),
        Path::new("/tmp/out.mov"),
    );
    assert_eq!(command.program, "ffmpeg");
    assert!(command.args.contains(&"-c".to_string()));
    assert!(command.args.contains(&"copy".to_string()));
    assert!(!command.args.join(" ").contains(';'));
}

#[test]
fn generates_reports() {
    let segments = vec![mk_segment_with_notes("Moment", 1_000, 2_500, "keeper")];
    let txt = rewrap_report_txt(Path::new("source.mov"), Path::new("out.mov"), &segments);
    let csv = rewrap_report_csv(Path::new("source.mov"), Path::new("out.mov"), &segments);
    assert!(txt.contains("keyframe-aligned stream copy"));
    assert!(csv.contains("Moment"));
}

#[test]
fn saves_and_loads_project() {
    let dir = tempdir().unwrap();
    let path = dir.path().join("project.seder-rewrap.json");
    let project = RewrapProject {
        version: CURRENT_PROJECT_VERSION,
        source_file: "source.mov".into(),
        output_file: "out.mov".into(),
        segments: vec![mk_segment("A", 0, 1_000)],
    };
    save_project(&path, &project).unwrap();
    assert_eq!(load_project(&path).unwrap(), project);
}

#[test]
fn keyframe_validation_tolerates_2ms_drift() {
    // Real keyframes from ffprobe round-tripped through f64 → i64 ms.
    let keyframes = mk_keyframes(&DRIFTED_KEYFRAMES);
    // User snapped via UI; due to FP rounding the segment landed 1ms away.
    let segment = mk_segment("Drifted", 999, 2_000);
    assert!(validate_segment(&segment, &keyframes).is_ok());
}

#[test]
fn keyframe_validation_rejects_off_by_more_than_tolerance() {
    let keyframes = mk_keyframes(&DRIFTED_KEYFRAMES);
    let segment = mk_segment("Off", 996, 2_001); // 4ms from 1000 — outside the 2ms tolerance
    assert!(validate_segment(&segment, &keyframes).is_err());
}

#[test]
fn ffmpeg_segment_args_use_duration_and_stable_seek() {
    let segment = mk_segment("A", 1_000, 2_500);
    let cmd = ffmpeg_segment_command(
        Path::new("/media/src.mov"),
        &segment,
        &mk_keyframes(&DEFAULT_KEYFRAMES),
        Path::new("/tmp/out.mov"),
    );
    let args = cmd.args.join(" ");
    // -t (duration) replaces -to (end-time) for version-stable semantics.
    assert!(cmd.args.contains(&"-t".to_string()));
    assert!(!cmd.args.contains(&"-to".to_string()));
    // Ad-hoc keyframe-only seek: don't decode unwanted frames.
    assert!(cmd.args.contains(&"-noaccurate_seek".to_string()));
    // Preserve source timestamps so the concat step lines up cleanly.
    assert!(cmd.args.contains(&"-copyts".to_string()));
    assert!(cmd.args.contains(&"-c".to_string()));
    assert!(cmd.args.contains(&"copy".to_string()));
    // Sanity-check the duration is computed (1500 ms = 00:00:01.500).
    assert!(args.contains("00:00:01.500"));
}

#[test]
fn project_loads_old_save_without_version_field() {
    // A project saved by an older build (pre-F9) had no `version` field.
    let project: RewrapProject = serde_json::from_str(LEGACY_PROJECT_JSON).unwrap();
    assert_eq!(project.version, CURRENT_PROJECT_VERSION);
    assert_eq!(project.source_file, "src.mov");
    assert_eq!(project.segments.len(), 1);
}

#[test]
fn ffmpeg_segment_snaps_drifted_input_to_actual_keyframes() {
    // Real keyframes round-tripped through f64 → i64 ms.
    let keyframes = mk_keyframes(&DRIFTED_KEYFRAMES);
    // User-supplied values 1ms below the real keyframes — within validation
    // tolerance, but if passed verbatim to ffmpeg's -ss with -noaccurate_seek
    // they would land on the *previous* keyframe and silently shift content.
    let segment = mk_segment("Drifted", 999, 2_000);
    let cmd = ffmpeg_segment_command(
        Path::new("/media/src.mov"),
        &segment,
        &keyframes,
        Path::new("/tmp/out.mov"),
    );
    let args = cmd.args.join(" ");
    // -ss should be 1.000 (snapped from 999), not 0.999, and duration should
    // be 1.001 (2001 - 1000), not 1.001 from a wrong base.
    assert!(args.contains("-ss 00:00:01.000"), "args: {}", args);
    assert!(args.contains("-t 00:00:01.001"), "args: {}", args);
    assert!(!args.contains("00:00:00.999"));
}

#[test]
fn known_keyframe_helper_uses_tolerance() {
    let keys = mk_keyframes(&DRIFTED_KEYFRAMES);
    assert!(is_known_keyframe(&keys, 1_002));
    assert!(is_known_keyframe(&keys, 998));
    assert!(!is_known_keyframe(&keys, 1_005));
}
