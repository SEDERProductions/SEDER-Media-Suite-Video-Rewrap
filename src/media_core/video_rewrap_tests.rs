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
fn ignores_invalid_ffprobe_keyframe_values() {
    let parsed = parse_ffprobe_keyframes("0.000000\nNaN\ninf\n-1.000000\n1.250000\n");
    assert_eq!(parsed, vec![0, 1_250]);
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
fn rejects_invalid_timecode_ranges() {
    assert!(parse_timecode_to_ms("-1").is_err());
    assert!(parse_timecode_to_ms("nan").is_err());
    assert!(parse_timecode_to_ms("00:60:00.000").is_err());
    assert!(parse_timecode_to_ms("00:00:60.000").is_err());
}

#[test]
fn snaps_to_nearest_keyframe() {
    let snap = nearest_keyframe(&mk_keyframes(&DEFAULT_KEYFRAMES), 2_100).unwrap();
    assert_eq!(snap.snapped_ms, 2_500);
    assert_eq!(snap.distance_ms, 400);
}

#[test]
fn keyframe_distance_handles_extreme_values() {
    let snap = nearest_keyframe(&[i64::MAX], i64::MIN).unwrap();
    assert_eq!(snap.snapped_ms, i64::MAX);
    assert_eq!(snap.distance_ms, i64::MAX);
    assert!(!is_known_keyframe(&[i64::MAX], i64::MIN));
}

#[test]
fn calculates_segment_and_total_duration() {
    let segments = vec![
        mk_segment("A", 1_000, 2_500),
        RewrapSegment {
            enabled: false,
            ..mk_segment("B", 2_500, 8_000)
        },
        mk_segment("C", 5_000, 8_000),
    ];
    assert_eq!(segment_duration(&segments[0]), 1_500);
    assert_eq!(total_enabled_duration(&segments), 4_500);
}

#[test]
fn reorders_segments() {
    let mut segments = vec![mk_segment("A", 0, 1_000), mk_segment("B", 1_000, 2_500)];
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
    let metadata_command = ffprobe_metadata_command(Path::new("/media/source.mov"));
    assert!(metadata_command
        .args
        .contains(&"-select_streams".to_string()));
    assert!(metadata_command.args.contains(&"v:0".to_string()));

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
fn ffmpeg_thumbnail_command_extracts_single_scaled_frame() {
    let cmd = ffmpeg_thumbnail_command(
        Path::new("/media/source.mov"),
        2_000,
        Path::new("/tmp/thumb.png"),
    );
    assert_eq!(cmd.program, "ffmpeg");
    // Exactly one frame, scaled down, written to the requested path.
    assert!(cmd.args.contains(&"-frames:v".to_string()));
    assert!(cmd.args.contains(&"1".to_string()));
    assert!(cmd.args.iter().any(|arg| arg.starts_with("scale=")));
    assert!(cmd.args.contains(&"/tmp/thumb.png".to_string()));
    // 2000 ms requests an input-side seek to 00:00:02.000.
    assert!(cmd.args.contains(&"00:00:02.000".to_string()));
    assert!(!cmd.args.join(" ").contains(';'));
}

#[test]
fn generates_reports() {
    let segments = vec![mk_segment_with_notes("Moment", 1_000, 2_500, "keeper")];
    let txt = rewrap_report_txt(
        Path::new("source.mov"),
        Path::new("out.mov"),
        &segments,
        ExportMode::ConcatSingle,
    );
    let csv = rewrap_report_csv(
        Path::new("source.mov"),
        Path::new("out.mov"),
        &segments,
        ExportMode::SeparateFiles,
    );
    assert!(txt.contains("concat single output"));
    assert!(csv.contains("Moment"));
    assert!(csv.contains("segment_output_path"));
    assert!(txt.contains("Output path:"));
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
    // Timestamps are NOT preserved — each segment's PTS starts at ~0
    // so the concat demuxer produces continuous output.
    assert!(!cmd.args.contains(&"-copyts".to_string()));
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
fn migrates_unversioned_project_to_current_version() {
    let project = parse_project_json(LEGACY_PROJECT_JSON).unwrap();
    assert_eq!(project.version, CURRENT_PROJECT_VERSION);
    assert_eq!(project.segments.len(), 1);
    assert_eq!(project.source_file, "src.mov");
}

#[test]
fn rejects_project_saved_by_newer_app() {
    // A future build saves with version=99; the current app should refuse.
    let future = r#"{ "version": 99, "source_file": "", "output_file": "", "segments": [] }"#;
    let err = parse_project_json(future).unwrap_err();
    let msg = err.to_string();
    assert!(
        msg.contains("newer version") || msg.contains("Update the app"),
        "expected newer-version error, got: {msg}"
    );
}

#[test]
fn roundtrips_migrated_project_through_save() {
    let dir = tempdir().unwrap();
    let path = dir.path().join("legacy.json");
    std::fs::write(&path, LEGACY_PROJECT_JSON).unwrap();
    let project = load_project(&path).unwrap();
    assert_eq!(project.version, CURRENT_PROJECT_VERSION);
    save_project(&path, &project).unwrap();
    let reread = load_project(&path).unwrap();
    assert_eq!(reread, project);
}

#[test]
fn parses_real_ffmpeg_version_strings() {
    // Real outputs from ffmpeg.org builds and distro packages.
    assert_eq!(
        parse_ffmpeg_version("ffmpeg version 6.1.1 Copyright (c) 2000-2023 the FFmpeg developers"),
        Some(SemVer {
            major: 6,
            minor: 1,
            patch: 1
        }),
    );
    assert_eq!(
        parse_ffmpeg_version("ffmpeg version n4.4 Copyright (c) ..."),
        Some(SemVer {
            major: 4,
            minor: 4,
            patch: 0
        }),
    );
    assert_eq!(
        parse_ffmpeg_version("ffmpeg version 7.0-static https://johnvansickle.com/ffmpeg/"),
        Some(SemVer {
            major: 7,
            minor: 0,
            patch: 0
        }),
    );
    // Git snapshot builds carry an N- prefix and no real numeric version —
    // we surface that as "unknown" rather than guessing.
    assert_eq!(
        parse_ffmpeg_version("ffmpeg version N-114000-g0123abcd Copyright ..."),
        None,
    );
}

#[test]
fn ffmpeg_compatibility_accepts_supported_versions() {
    let compat = ffmpeg_compatibility("ffmpeg version 6.0 Copyright (c) ...");
    assert!(compat.compatible);
    assert_eq!(compat.detected.unwrap().major, 6);
}

#[test]
fn ffmpeg_compatibility_flags_old_versions() {
    let compat = ffmpeg_compatibility("ffmpeg version 3.4.8 Copyright (c) ...");
    assert!(!compat.compatible);
    assert!(compat.message.contains("older than"));
}

#[test]
fn ffmpeg_compatibility_reports_unparseable_output() {
    let compat = ffmpeg_compatibility("totally unrelated text");
    assert!(!compat.compatible);
    assert!(compat.detected.is_none());
}

#[test]
fn semver_comparison_orders_correctly() {
    let a = parse_semver("1.2.3").unwrap();
    let b = parse_semver("1.2.4").unwrap();
    let c = parse_semver("1.10.0").unwrap();
    assert!(a < b);
    assert!(b < c);
    assert_eq!(
        parse_semver("v0.1.29"),
        Some(SemVer {
            major: 0,
            minor: 1,
            patch: 29
        })
    );
    assert!(parse_semver("not-a-version").is_none());
}

#[test]
fn update_check_detects_newer_release() {
    let latest = r#"{"version":"0.2.0","url":"https://example.invalid/release"}"#;
    let info = evaluate_update(latest, "0.1.29").unwrap();
    assert!(info.update_available);
    assert_eq!(info.url, "https://example.invalid/release");
    assert!(info.message.contains("Update available"));
}

#[test]
fn update_check_reports_up_to_date() {
    let latest = r#"{"version":"0.1.29"}"#;
    let info = evaluate_update(latest, "0.1.29").unwrap();
    assert!(!info.update_available);
    assert_eq!(info.url, "");
    assert!(info.message.contains("Up to date"));
}

#[test]
fn update_check_ignores_older_advertised_version() {
    let latest = r#"{"version":"0.0.5"}"#;
    let info = evaluate_update(latest, "0.1.29").unwrap();
    assert!(!info.update_available);
}

#[test]
fn update_check_rejects_malformed_payload() {
    assert!(evaluate_update("not json", "0.1.29").is_err());
    assert!(evaluate_update(r#"{"foo":"bar"}"#, "0.1.29").is_err());
    assert!(evaluate_update(r#"{"version":"not-a-version"}"#, "0.1.29").is_err());
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

#[test]
fn preflight_passes_for_matching_container_family() {
    let metadata = VideoMetadata {
        filename: "src.mov".into(),
        duration_ms: Some(8_000),
        container: Some("mov,mp4,m4a,3gp,3g2,mj2".into()),
        width: Some(1920),
        height: Some(1080),
        codec: Some("h264".into()),
        frame_rate: Some("30000/1001".into()),
        file_size: 10,
    };
    let segments = vec![RewrapSegment {
        name: "A".into(),
        in_ms: 0,
        out_ms: 1_000,
        notes: String::new(),
        enabled: true,
    }];
    let preflight =
        rewrap_preflight(&metadata, &segments, &[0, 1_000], Path::new("/tmp/out.mp4")).unwrap();
    assert!(preflight.container_match);
    assert_eq!(preflight.container_extension, "mp4");
}

#[test]
fn preflight_passes_for_any_output_container() {
    let metadata = VideoMetadata {
        filename: "src.mov".into(),
        duration_ms: Some(8_000),
        container: Some("mov,mp4,m4a,3gp,3g2,mj2".into()),
        width: Some(1920),
        height: Some(1080),
        codec: Some("h264".into()),
        frame_rate: Some("30000/1001".into()),
        file_size: 10,
    };
    let segments = vec![RewrapSegment {
        name: "A".into(),
        in_ms: 0,
        out_ms: 1_000,
        notes: String::new(),
        enabled: true,
    }];
    let preflight =
        rewrap_preflight(&metadata, &segments, &[0, 1_000], Path::new("/tmp/out.mkv")).unwrap();
    assert!(preflight.container_match);
    assert_eq!(preflight.container_extension, "mkv");
    assert!(preflight.guidance.iter().any(|g| g.contains("stream-copy")));
}
