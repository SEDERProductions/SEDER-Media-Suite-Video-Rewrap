use anyhow::{Context, Result};
use serde::{Deserialize, Serialize};
use std::fs;
use std::path::{Path, PathBuf};

#[derive(Debug, Clone, Default, Serialize, Deserialize, PartialEq, Eq)]
pub struct VideoMetadata {
    pub filename: String,
    pub duration_ms: Option<i64>,
    pub container: Option<String>,
    pub width: Option<u32>,
    pub height: Option<u32>,
    pub codec: Option<String>,
    pub frame_rate: Option<String>,
    pub file_size: u64,
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct RewrapSegment {
    #[serde(default)]
    pub name: String,
    #[serde(default)]
    pub in_ms: i64,
    #[serde(default)]
    pub out_ms: i64,
    #[serde(default)]
    pub notes: String,
    #[serde(default = "enabled_default")]
    pub enabled: bool,
}

fn enabled_default() -> bool {
    true
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct RewrapProject {
    #[serde(default = "project_version_default")]
    pub version: u32,
    #[serde(default)]
    pub source_file: String,
    #[serde(default)]
    pub output_file: String,
    #[serde(default)]
    pub segments: Vec<RewrapSegment>,
}

pub const CURRENT_PROJECT_VERSION: u32 = 1;

fn project_version_default() -> u32 {
    CURRENT_PROJECT_VERSION
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct SnapResult {
    pub requested_ms: i64,
    pub snapped_ms: i64,
    pub distance_ms: i64,
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct ProcessCommand {
    pub program: String,
    pub args: Vec<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct RewrapPreflight {
    pub container_extension: String,
    pub container_match: bool,
    pub enabled_segment_count: usize,
    pub no_reencode_fallback: bool,
    pub guidance: Vec<String>,
}

fn container_extension_from_path(output: &Path) -> String {
    output
        .extension()
        .and_then(|v| v.to_str())
        .unwrap_or("")
        .to_ascii_lowercase()
}

fn container_family_for_extension(ext: &str) -> Option<&'static str> {
    match ext {
        "mov" => Some("mov"),
        "mp4" | "m4v" => Some("mp4"),
        "mkv" => Some("matroska"),
        _ => None,
    }
}

fn metadata_has_family(container: Option<&str>, family: &str) -> bool {
    let Some(container) = container else {
        return false;
    };
    container
        .split(',')
        .any(|c| c.trim().eq_ignore_ascii_case(family))
}

pub fn rewrap_preflight(
    metadata: &VideoMetadata,
    segments: &[RewrapSegment],
    keyframes: &[i64],
    output: &Path,
) -> Result<RewrapPreflight> {
    validate_segments(segments, keyframes)?;
    let enabled_segment_count = segments.iter().filter(|s| s.enabled).count();
    if enabled_segment_count == 0 {
        anyhow::bail!("No enabled segments to export. Enable at least one segment.");
    }

    let ext = container_extension_from_path(output);
    let family = container_family_for_extension(&ext).with_context(|| {
        format!("Output container '.{}' is not supported for stream-copy rewrap. Use .mov, .mp4/.m4v, or .mkv.", ext)
    })?;

    if !metadata_has_family(metadata.container.as_deref(), family) {
        anyhow::bail!(
            "Preflight failed: source container '{}' does not advertise {} compatibility for stream-copy output '.{}'. No re-encode fallback is available. Adjust container choice, stream layout, or source normalization before exporting.",
            metadata.container.clone().unwrap_or_else(|| "unknown".into()),
            family,
            ext
        );
    }

    Ok(RewrapPreflight {
        container_extension: ext,
        container_match: true,
        enabled_segment_count,
        no_reencode_fallback: true,
        guidance: vec![
            "No re-encode fallback is available in this export path.".into(),
            "Adjust container choice (mov/mp4/m4v/mkv) so it matches the source container family.".into(),
            "Adjust stream layout (for example remove unsupported attachments/data streams for target container).".into(),
            "Normalize the source with an explicit transcode/remux pass before using segment rewrap.".into(),
        ],
    })
}

pub fn parse_timecode_to_ms(value: &str) -> Result<i64> {
    let trimmed = value.trim();
    if trimmed.is_empty() {
        anyhow::bail!("Timecode is empty");
    }
    if !trimmed.contains(':') {
        let seconds = trimmed.parse::<f64>().context("Invalid seconds value")?;
        return Ok((seconds * 1000.0).round() as i64);
    }
    let parts = trimmed.split(':').collect::<Vec<_>>();
    if parts.len() != 3 {
        anyhow::bail!("Use HH:MM:SS.mmm timecode");
    }
    let hours = parts[0].parse::<i64>().context("Invalid hours")?;
    let minutes = parts[1].parse::<i64>().context("Invalid minutes")?;
    let seconds = parts[2].parse::<f64>().context("Invalid seconds")?;
    Ok((((hours * 60 + minutes) * 60) * 1000) + (seconds * 1000.0).round() as i64)
}

pub fn format_ms(ms: i64) -> String {
    let safe = ms.max(0);
    let hours = safe / 3_600_000;
    let minutes = (safe % 3_600_000) / 60_000;
    let seconds = (safe % 60_000) / 1000;
    let millis = safe % 1000;
    format!("{hours:02}:{minutes:02}:{seconds:02}.{millis:03}")
}

pub fn parse_ffprobe_keyframes(output: &str) -> Vec<i64> {
    let mut frames = output
        .lines()
        .filter_map(|line| {
            let trimmed = line.trim();
            if trimmed.is_empty() || trimmed == "N/A" {
                return None;
            }
            trimmed
                .parse::<f64>()
                .ok()
                .map(|seconds| (seconds * 1000.0).round() as i64)
        })
        .collect::<Vec<_>>();
    frames.sort_unstable();
    frames.dedup();
    frames
}

pub fn parse_ffprobe_metadata(output: &str, source: &Path, file_size: u64) -> VideoMetadata {
    let mut metadata = VideoMetadata {
        filename: source
            .file_name()
            .and_then(|v| v.to_str())
            .unwrap_or_default()
            .to_string(),
        file_size,
        ..VideoMetadata::default()
    };
    let mut format_name = None;
    let mut duration = None;
    let mut width = None;
    let mut height = None;
    let mut codec = None;
    let mut avg_frame_rate = None;
    let mut r_frame_rate = None;

    for line in output.lines() {
        let Some((key, value)) = line.split_once('=') else {
            continue;
        };
        let key = key.trim();
        let value = value.trim();
        if value.is_empty() {
            continue;
        }
        match key {
            "format_name" => format_name = Some(value.to_string()),
            "duration" => duration = value.parse::<f64>().ok(),
            "width" => width = value.parse::<u32>().ok(),
            "height" => height = value.parse::<u32>().ok(),
            "codec_name" => codec = Some(value.to_string()),
            "avg_frame_rate" if value != "0/0" => avg_frame_rate = Some(value.to_string()),
            "r_frame_rate" if value != "0/0" => r_frame_rate = Some(value.to_string()),
            _ => {}
        }
    }

    metadata.container = format_name;
    metadata.duration_ms = duration.map(|seconds| (seconds * 1000.0).round() as i64);
    metadata.width = width;
    metadata.height = height;
    metadata.codec = codec;
    metadata.frame_rate = avg_frame_rate.or(r_frame_rate);
    metadata
}

pub fn nearest_keyframe(keyframes: &[i64], requested_ms: i64) -> Option<SnapResult> {
    keyframes
        .iter()
        .copied()
        .min_by_key(|candidate| (candidate - requested_ms).abs())
        .map(|snapped_ms| SnapResult {
            requested_ms,
            snapped_ms,
            distance_ms: (snapped_ms - requested_ms).abs(),
        })
}

pub fn segment_duration(segment: &RewrapSegment) -> i64 {
    (segment.out_ms - segment.in_ms).max(0)
}

pub fn total_enabled_duration(segments: &[RewrapSegment]) -> i64 {
    segments
        .iter()
        .filter(|segment| segment.enabled)
        .map(segment_duration)
        .sum()
}

pub const KEYFRAME_TOLERANCE_MS: i64 = 2;

pub fn is_known_keyframe(keyframes: &[i64], ms: i64) -> bool {
    keyframes
        .iter()
        .any(|candidate| (candidate - ms).abs() <= KEYFRAME_TOLERANCE_MS)
}

pub fn validate_segment(segment: &RewrapSegment, keyframes: &[i64]) -> Result<()> {
    if segment.in_ms >= segment.out_ms {
        anyhow::bail!(
            "Segment '{}' has an out point before its in point",
            segment.name
        );
    }
    if !is_known_keyframe(keyframes, segment.in_ms) {
        anyhow::bail!(
            "Segment '{}' in point is not a detected keyframe",
            segment.name
        );
    }
    if !is_known_keyframe(keyframes, segment.out_ms) {
        anyhow::bail!(
            "Segment '{}' out point is not a detected keyframe",
            segment.name
        );
    }
    Ok(())
}

pub fn validate_segments(segments: &[RewrapSegment], keyframes: &[i64]) -> Result<()> {
    for segment in segments.iter().filter(|segment| segment.enabled) {
        validate_segment(segment, keyframes)?;
    }
    Ok(())
}

pub fn move_segment(segments: &mut [RewrapSegment], index: usize, direction: i32) {
    if direction < 0 && index > 0 {
        segments.swap(index, index - 1);
    } else if direction > 0 && index + 1 < segments.len() {
        segments.swap(index, index + 1);
    }
}

pub fn ffprobe_keyframe_command(source: &Path) -> ProcessCommand {
    ProcessCommand {
        program: "ffprobe".into(),
        args: vec![
            "-v".into(),
            "error".into(),
            "-select_streams".into(),
            "v:0".into(),
            "-skip_frame".into(),
            "nokey".into(),
            "-show_entries".into(),
            "frame=best_effort_timestamp_time".into(),
            "-of".into(),
            "default=noprint_wrappers=1:nokey=1".into(),
            source.to_string_lossy().to_string(),
        ],
    }
}

pub fn ffprobe_metadata_command(source: &Path) -> ProcessCommand {
    ProcessCommand {
        program: "ffprobe".into(),
        args: vec![
            "-v".into(),
            "error".into(),
            "-show_entries".into(),
            "format=format_name,duration".into(),
            "-select_streams".into(),
            "v:0".into(),
            "-show_entries".into(),
            "stream=codec_name,width,height,avg_frame_rate,r_frame_rate".into(),
            "-of".into(),
            "default=noprint_wrappers=1".into(),
            source.to_string_lossy().to_string(),
        ],
    }
}

fn snap_to_keyframe(keyframes: &[i64], ms: i64) -> i64 {
    nearest_keyframe(keyframes, ms)
        .map(|snap| snap.snapped_ms)
        .unwrap_or(ms)
}

pub fn ffmpeg_segment_command(
    source: &Path,
    segment: &RewrapSegment,
    keyframes: &[i64],
    output: &Path,
) -> ProcessCommand {
    // Snap to the actual keyframe before passing to ffmpeg. With -ss before -i and
    // -noaccurate_seek, ffmpeg fast-seeks to the keyframe at-or-before the requested
    // timestamp; passing a value 1ms below a real keyframe would silently land on the
    // previous keyframe. The validate step tolerates ±KEYFRAME_TOLERANCE_MS drift, so
    // the seek value must be the matched keyframe, not the user-supplied input.
    let in_ms = snap_to_keyframe(keyframes, segment.in_ms);
    let out_ms = snap_to_keyframe(keyframes, segment.out_ms);
    let duration_ms = (out_ms - in_ms).max(0);
    ProcessCommand {
        program: "ffmpeg".into(),
        args: vec![
            "-y".into(),
            "-ss".into(),
            format_ms(in_ms),
            "-noaccurate_seek".into(),
            "-i".into(),
            source.to_string_lossy().to_string(),
            "-map".into(),
            "0".into(),
            "-c".into(),
            "copy".into(),
            "-t".into(),
            format_ms(duration_ms),
            "-copyts".into(),
            "-avoid_negative_ts".into(),
            "make_zero".into(),
            output.to_string_lossy().to_string(),
        ],
    }
}

pub fn ffmpeg_concat_command(list_file: &Path, output: &Path) -> ProcessCommand {
    ProcessCommand {
        program: "ffmpeg".into(),
        args: vec![
            "-y".into(),
            "-f".into(),
            "concat".into(),
            "-safe".into(),
            "0".into(),
            "-i".into(),
            list_file.to_string_lossy().to_string(),
            "-c".into(),
            "copy".into(),
            output.to_string_lossy().to_string(),
        ],
    }
}

pub fn ffplay_preview_command(source: &Path, start_ms: i64) -> ProcessCommand {
    ProcessCommand {
        program: "ffplay".into(),
        args: vec![
            "-autoexit".into(),
            "-ss".into(),
            format_ms(start_ms),
            source.to_string_lossy().to_string(),
        ],
    }
}

pub fn save_project(path: &Path, project: &RewrapProject) -> Result<()> {
    let json = serde_json::to_string_pretty(project)?;
    fs::write(path, json).with_context(|| format!("Unable to write {}", path.display()))
}

pub fn load_project(path: &Path) -> Result<RewrapProject> {
    let json =
        fs::read_to_string(path).with_context(|| format!("Unable to read {}", path.display()))?;
    Ok(serde_json::from_str(&json)?)
}

fn csv_cell(value: impl AsRef<str>) -> String {
    format!("\"{}\"", value.as_ref().replace('"', "\"\""))
}

pub fn rewrap_report_txt(source: &Path, output: &Path, segments: &[RewrapSegment]) -> String {
    let enabled = segments
        .iter()
        .filter(|segment| segment.enabled)
        .collect::<Vec<_>>();
    let mut report = format!(
        "SEDER Media Suite Video Rewrap Report\nSource file: {}\nOutput file: {}\nExport mode: keyframe-aligned stream copy\nExport timestamp: {}\nTotal selected duration: {}\n\n",
        source.display(),
        output.display(),
        current_timestamp(),
        format_ms(total_enabled_duration(segments))
    );
    for segment in enabled {
        report.push_str(&format!(
            "{}\nIn keyframe: {}\nOut keyframe: {}\nDuration: {}\nNotes: {}\n\n",
            segment.name,
            format_ms(segment.in_ms),
            format_ms(segment.out_ms),
            format_ms(segment_duration(segment)),
            segment.notes
        ));
    }
    report
}

pub fn rewrap_report_csv(source: &Path, output: &Path, segments: &[RewrapSegment]) -> String {
    let mut out = String::from("\"source_file\",\"output_file\",\"export_mode\",\"segment_name\",\"in_keyframe_time\",\"out_keyframe_time\",\"duration\",\"notes\",\"total_selected_duration\",\"export_timestamp\"\n");
    let timestamp = current_timestamp();
    let total = format_ms(total_enabled_duration(segments));
    for segment in segments.iter().filter(|segment| segment.enabled) {
        let row = [
            source.display().to_string(),
            output.display().to_string(),
            "keyframe-aligned stream copy".to_string(),
            segment.name.clone(),
            format_ms(segment.in_ms),
            format_ms(segment.out_ms),
            format_ms(segment_duration(segment)),
            segment.notes.clone(),
            total.clone(),
            timestamp.clone(),
        ];
        out.push_str(&row.iter().map(csv_cell).collect::<Vec<_>>().join(","));
        out.push('\n');
    }
    out
}

fn current_timestamp() -> String {
    chrono::Utc::now().to_rfc3339()
}

pub fn temp_segment_path(temp_dir: &Path, index: usize, extension: &str) -> PathBuf {
    temp_dir.join(format!("segment-{index:04}.{extension}"))
}
