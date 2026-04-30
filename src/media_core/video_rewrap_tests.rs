use super::video_rewrap::*;
use std::path::Path;
use tempfile::tempdir;

fn keys() -> Vec<i64> {
    vec![0, 1_000, 2_500, 5_000, 8_000]
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
    let snap = nearest_keyframe(&keys(), 2_100).unwrap();
    assert_eq!(snap.snapped_ms, 2_500);
    assert_eq!(snap.distance_ms, 400);
}

#[test]
fn calculates_segment_and_total_duration() {
    let segments = vec![
        RewrapSegment {
            name: "A".into(),
            in_ms: 1_000,
            out_ms: 2_500,
            notes: String::new(),
            enabled: true,
        },
        RewrapSegment {
            name: "B".into(),
            in_ms: 2_500,
            out_ms: 8_000,
            notes: String::new(),
            enabled: false,
        },
        RewrapSegment {
            name: "C".into(),
            in_ms: 5_000,
            out_ms: 8_000,
            notes: String::new(),
            enabled: true,
        },
    ];
    assert_eq!(segment_duration(&segments[0]), 1_500);
    assert_eq!(total_enabled_duration(&segments), 4_500);
}

#[test]
fn reorders_segments() {
    let mut segments = vec![
        RewrapSegment {
            name: "A".into(),
            in_ms: 0,
            out_ms: 1_000,
            notes: String::new(),
            enabled: true,
        },
        RewrapSegment {
            name: "B".into(),
            in_ms: 1_000,
            out_ms: 2_500,
            notes: String::new(),
            enabled: true,
        },
    ];
    move_segment(&mut segments, 1, -1);
    assert_eq!(segments[0].name, "B");
}

#[test]
fn validates_invalid_segments() {
    let bad = RewrapSegment {
        name: "Bad".into(),
        in_ms: 1_200,
        out_ms: 2_500,
        notes: String::new(),
        enabled: true,
    };
    assert!(validate_segment(&bad, &keys()).is_err());
    let reversed = RewrapSegment {
        name: "Reversed".into(),
        in_ms: 2_500,
        out_ms: 1_000,
        notes: String::new(),
        enabled: true,
    };
    assert!(validate_segment(&reversed, &keys()).is_err());
}

#[test]
fn generates_ffmpeg_commands_without_shell_strings() {
    let segment = RewrapSegment {
        name: "A".into(),
        in_ms: 1_000,
        out_ms: 2_500,
        notes: String::new(),
        enabled: true,
    };
    let command = ffmpeg_segment_command(
        Path::new("/media/source.mov"),
        &segment,
        Path::new("/tmp/out.mov"),
    );
    assert_eq!(command.program, "ffmpeg");
    assert!(command.args.contains(&"-c".to_string()));
    assert!(command.args.contains(&"copy".to_string()));
    assert!(!command.args.join(" ").contains(';'));
}

#[test]
fn generates_reports() {
    let segments = vec![RewrapSegment {
        name: "Moment".into(),
        in_ms: 1_000,
        out_ms: 2_500,
        notes: "keeper".into(),
        enabled: true,
    }];
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
        source_file: "source.mov".into(),
        output_file: "out.mov".into(),
        segments: vec![RewrapSegment {
            name: "A".into(),
            in_ms: 0,
            out_ms: 1_000,
            notes: String::new(),
            enabled: true,
        }],
    };
    save_project(&path, &project).unwrap();
    assert_eq!(load_project(&path).unwrap(), project);
}
