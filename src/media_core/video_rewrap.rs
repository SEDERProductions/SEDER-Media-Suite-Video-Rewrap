use anyhow::{Context, Result};
use serde::{Deserialize, Serialize};
use serde_json::Value;
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

#[derive(Debug, Clone, Copy, Default, Serialize, Deserialize, PartialEq, Eq)]
#[serde(rename_all = "snake_case")]
pub enum ExportMode {
    #[default]
    ConcatSingle,
    SeparateFiles,
}

impl ExportMode {
    pub fn label(self) -> &'static str {
        match self {
            Self::ConcatSingle => "concat single output",
            Self::SeparateFiles => "separate files output",
        }
    }
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

pub fn rewrap_preflight(
    _metadata: &VideoMetadata,
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
    let mut guidance = vec!["No re-encode fallback is available in this export path.".into()];

    guidance.push(format!(
        "Output container '.{}' will be used with stream-copy. If ffmpeg reports a muxer error, try a different container format.",
        if ext.is_empty() { "mov" } else { &ext }
    ));

    Ok(RewrapPreflight {
        container_extension: ext,
        container_match: true,
        enabled_segment_count,
        no_reencode_fallback: true,
        guidance,
    })
}

fn finite_non_negative_seconds(seconds: f64, field: &str) -> Result<f64> {
    if !seconds.is_finite() || seconds < 0.0 {
        anyhow::bail!("{field} must be a finite non-negative value");
    }
    Ok(seconds)
}

pub fn parse_timecode_to_ms(value: &str) -> Result<i64> {
    let trimmed = value.trim();
    if trimmed.is_empty() {
        anyhow::bail!("Timecode is empty");
    }
    if !trimmed.contains(':') {
        let seconds = finite_non_negative_seconds(
            trimmed.parse::<f64>().context("Invalid seconds value")?,
            "Seconds",
        )?;
        return Ok((seconds * 1000.0).round() as i64);
    }
    let mut parts = trimmed.splitn(3, ':');
    let hours = parts
        .next()
        .context("Use HH:MM:SS.mmm timecode")?
        .parse::<i64>()
        .context("Invalid hours")?;
    if hours < 0 {
        anyhow::bail!("Hours must be non-negative");
    }
    let minutes = parts
        .next()
        .context("Use HH:MM:SS.mmm timecode")?
        .parse::<i64>()
        .context("Invalid minutes")?;
    if !(0..60).contains(&minutes) {
        anyhow::bail!("Minutes must be between 0 and 59");
    }
    let seconds = finite_non_negative_seconds(
        parts
            .next()
            .context("Use HH:MM:SS.mmm timecode")?
            .parse::<f64>()
            .context("Invalid seconds")?,
        "Seconds",
    )?;
    if seconds >= 60.0 {
        anyhow::bail!("Seconds must be less than 60");
    }
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
                .filter(|seconds| seconds.is_finite() && *seconds >= 0.0)
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
    for line in output.lines() {
        let Some((key, value)) = line.split_once('=') else {
            continue;
        };
        let value = value.trim();
        match key.trim() {
            "format_name" => metadata.container = Some(value.to_string()),
            "duration" => {
                metadata.duration_ms = value
                    .parse::<f64>()
                    .ok()
                    .map(|seconds| (seconds * 1000.0).round() as i64);
            }
            "width" => metadata.width = value.parse::<u32>().ok(),
            "height" => metadata.height = value.parse::<u32>().ok(),
            "codec_name" => metadata.codec = Some(value.to_string()),
            "avg_frame_rate" | "r_frame_rate"
                if metadata.frame_rate.is_none() && value != "0/0" =>
            {
                metadata.frame_rate = Some(value.to_string());
            }
            _ => {}
        }
    }
    metadata
}

fn nearest_keyframe_index(keyframes: &[i64], requested_ms: i64) -> Option<usize> {
    if keyframes.is_empty() {
        return None;
    }
    let idx = match keyframes.binary_search(&requested_ms) {
        Ok(i) => i,
        Err(0) => 0,
        Err(i) if i >= keyframes.len() => keyframes.len() - 1,
        Err(i) => {
            if requested_ms.abs_diff(keyframes[i - 1]) <= requested_ms.abs_diff(keyframes[i]) {
                i - 1
            } else {
                i
            }
        }
    };
    Some(idx)
}

pub fn nearest_keyframe(keyframes: &[i64], requested_ms: i64) -> Option<SnapResult> {
    let idx = nearest_keyframe_index(keyframes, requested_ms)?;
    let snapped_ms = keyframes[idx];
    Some(SnapResult {
        requested_ms,
        snapped_ms,
        distance_ms: snapped_ms.abs_diff(requested_ms).min(i64::MAX as u64) as i64,
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
    nearest_keyframe(keyframes, ms).is_some_and(|snap| snap.distance_ms <= KEYFRAME_TOLERANCE_MS)
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
            "-v".into(), "error".into(),
            "-select_streams".into(), "v:0".into(),
            "-show_entries".into(), "format=format_name,duration:stream=codec_name,width,height,avg_frame_rate,r_frame_rate".into(),
            "-of".into(), "default=noprint_wrappers=1".into(),
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
    parse_project_json(&json)
}

/// Parses raw project JSON, running any necessary version migrations so the
/// returned project is always at `CURRENT_PROJECT_VERSION`. Rejects projects
/// saved by a newer build than this one understands.
pub fn parse_project_json(json: &str) -> Result<RewrapProject> {
    let value: Value = serde_json::from_str(json).context("Invalid project JSON")?;
    let migrated = migrate_project_value(value)?;
    serde_json::from_value(migrated).context("Project JSON does not match the expected schema")
}

fn migrate_project_value(mut value: Value) -> Result<Value> {
    let object = value
        .as_object_mut()
        .context("Project JSON must be an object")?;
    let version = object
        .get("version")
        .and_then(|v| v.as_u64())
        .map(|v| v as u32)
        .unwrap_or(0);

    if version > CURRENT_PROJECT_VERSION {
        anyhow::bail!(
            "Project was saved by a newer version (file version {version}, expected at most {CURRENT_PROJECT_VERSION}). Update the app to open this project."
        );
    }

    // v0 → v1: project predates the `version` field. Defaults already applied
    // via serde, but stamp the current version so re-saves are unambiguous.
    if version < 1 {
        object.insert("version".into(), Value::from(CURRENT_PROJECT_VERSION));
    }

    // v0 projects also predated the `enabled` field. Without an
    // explicit value, serde's `default = "enabled_default"` (which
    // returns `true`) would silently re-enable any segment the user
    // had disabled. Stamp `enabled: true` explicitly so the field is
    // materialised for v0 projects and re-saves are deterministic.
    if let Some(segments) = object.get_mut("segments").and_then(|v| v.as_array_mut()) {
        for segment in segments.iter_mut() {
            if let Some(obj) = segment.as_object_mut() {
                if !obj.contains_key("enabled") {
                    obj.insert("enabled".into(), Value::Bool(true));
                }
            }
        }
    }

    Ok(value)
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Serialize, Deserialize)]
pub struct SemVer {
    pub major: u32,
    pub minor: u32,
    pub patch: u32,
}

impl SemVer {
    pub fn zero() -> Self {
        Self {
            major: 0,
            minor: 0,
            patch: 0,
        }
    }
}

/// Parses a semver-ish string of the form `MAJOR.MINOR.PATCH`, ignoring any
/// pre-release or build suffix (e.g. `1.2.3-rc1+abc` → `1.2.3`). Also tolerates
/// a leading `v` (e.g. `v1.2.3`). Returns `None` for unrecognized inputs.
pub fn parse_semver(value: &str) -> Option<SemVer> {
    let trimmed = value.trim().trim_start_matches('v');
    let core = trimmed.split(['-', '+', ' ']).next()?;
    let mut parts = core.split('.');
    let major: u32 = parts.next()?.parse().ok()?;
    let minor: u32 = parts.next().unwrap_or("0").parse().ok()?;
    let patch: u32 = parts.next().unwrap_or("0").parse().ok()?;
    Some(SemVer {
        major,
        minor,
        patch,
    })
}

/// Extracts the FFmpeg version from the first line of `ffmpeg -version` output.
/// Real ffmpeg output looks like `ffmpeg version 6.1.1 Copyright (c) ...` or
/// `ffmpeg version n4.4 Copyright ...` for distro builds. Returns `None` if no
/// recognizable version token can be parsed.
pub fn parse_ffmpeg_version(output: &str) -> Option<SemVer> {
    let first = output.lines().next()?.trim();
    let rest = first.strip_prefix("ffmpeg version")?.trim();
    // The version token is the first whitespace-delimited segment.
    let token = rest.split_whitespace().next()?;
    // Distro builds prefix with `n` (`n4.4`) or `N-` (git snapshots). Strip those.
    let stripped = token.trim_start_matches(['n', 'N', 'v']);
    parse_semver(stripped)
}

pub const MIN_FFMPEG_VERSION: SemVer = SemVer {
    major: 4,
    minor: 0,
    patch: 0,
};

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct FfmpegCompatibility {
    pub detected: Option<SemVer>,
    pub minimum: SemVer,
    pub compatible: bool,
    pub message: String,
}

pub fn ffmpeg_compatibility(output: &str) -> FfmpegCompatibility {
    let detected = parse_ffmpeg_version(output);
    let compatible = detected.is_some_and(|v| v >= MIN_FFMPEG_VERSION);
    let message = match detected {
        Some(v) if compatible => format!(
            "Detected FFmpeg {}.{}.{} (>= {}.{}.{} required).",
            v.major,
            v.minor,
            v.patch,
            MIN_FFMPEG_VERSION.major,
            MIN_FFMPEG_VERSION.minor,
            MIN_FFMPEG_VERSION.patch,
        ),
        Some(v) => format!(
            "Detected FFmpeg {}.{}.{} which is older than the required {}.{}.{}. Upgrade FFmpeg or point the app at a newer binary.",
            v.major,
            v.minor,
            v.patch,
            MIN_FFMPEG_VERSION.major,
            MIN_FFMPEG_VERSION.minor,
            MIN_FFMPEG_VERSION.patch,
        ),
        None => "Unable to determine the FFmpeg version. Verify the binary works on its own and try again.".to_string(),
    };
    FfmpegCompatibility {
        detected,
        minimum: MIN_FFMPEG_VERSION,
        compatible,
        message,
    }
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct UpdateInfo {
    pub current: SemVer,
    pub latest: Option<SemVer>,
    pub update_available: bool,
    pub url: String,
    pub message: String,
}

/// Compares a `latest.json` payload (as a JSON string with `version` + optional
/// `url`) against the supplied current version. Returns an `UpdateInfo`
/// describing whether an upgrade is available.
pub fn evaluate_update(latest_json: &str, current: &str) -> Result<UpdateInfo> {
    let current_version =
        parse_semver(current).context("Current app version is not a valid semver")?;
    let parsed: Value =
        serde_json::from_str(latest_json).context("latest.json is not valid JSON")?;
    let object = parsed
        .as_object()
        .context("latest.json must be a JSON object")?;
    let latest_str = object
        .get("version")
        .and_then(|v| v.as_str())
        .context("latest.json is missing a \"version\" field")?;
    let latest = parse_semver(latest_str)
        .with_context(|| format!("latest.json \"version\" is not a valid semver: {latest_str}"))?;
    let url = object
        .get("url")
        .and_then(|v| v.as_str())
        .unwrap_or("")
        .to_string();
    let update_available = latest > current_version;
    let message = if update_available {
        format!(
            "Update available: {}.{}.{} (currently {}.{}.{}).",
            latest.major,
            latest.minor,
            latest.patch,
            current_version.major,
            current_version.minor,
            current_version.patch
        )
    } else {
        format!(
            "Up to date ({}.{}.{}).",
            current_version.major, current_version.minor, current_version.patch
        )
    };
    Ok(UpdateInfo {
        current: current_version,
        latest: Some(latest),
        update_available,
        url,
        message,
    })
}

fn csv_cell(value: impl AsRef<str>) -> String {
    format!("\"{}\"", value.as_ref().replace('"', "\"\""))
}

pub fn rewrap_report_txt(
    source: &Path,
    output: &Path,
    segments: &[RewrapSegment],
    export_mode: ExportMode,
) -> String {
    let enabled = segments
        .iter()
        .filter(|segment| segment.enabled)
        .collect::<Vec<_>>();
    let mut report = format!(
        "SEDER Media Suite Video Rewrap Report\nSource file: {}\nOutput file: {}\nExport mode: {}\nExport timestamp: {}\nTotal selected duration: {}\n\n",
        source.display(),
        output.display(),
        export_mode.label(),
        current_timestamp(),
        format_ms(total_enabled_duration(segments))
    );
    for (index, segment) in enabled.iter().enumerate() {
        let output_path = match export_mode {
            ExportMode::ConcatSingle => output.display().to_string(),
            ExportMode::SeparateFiles => {
                temp_segment_path(output, index + 1, &output_extension(output))
                    .display()
                    .to_string()
            }
        };
        report.push_str(&format!(
            "{}\nIn keyframe: {}\nOut keyframe: {}\nDuration: {}\nOutput path: {}\nNotes: {}\n\n",
            segment.name,
            format_ms(segment.in_ms),
            format_ms(segment.out_ms),
            format_ms(segment_duration(segment)),
            output_path,
            segment.notes
        ));
    }
    report
}

pub fn rewrap_report_csv(
    source: &Path,
    output: &Path,
    segments: &[RewrapSegment],
    export_mode: ExportMode,
) -> String {
    let mut out = String::from("\"source_file\",\"output_file\",\"export_mode\",\"segment_name\",\"segment_output_path\",\"in_keyframe_time\",\"out_keyframe_time\",\"duration\",\"notes\",\"total_selected_duration\",\"export_timestamp\"\n");
    let timestamp = current_timestamp();
    let total = format_ms(total_enabled_duration(segments));
    for (index, segment) in segments
        .iter()
        .filter(|segment| segment.enabled)
        .enumerate()
    {
        let segment_output_path = match export_mode {
            ExportMode::ConcatSingle => output.display().to_string(),
            ExportMode::SeparateFiles => {
                temp_segment_path(output, index + 1, &output_extension(output))
                    .display()
                    .to_string()
            }
        };
        let row = [
            source.display().to_string(),
            output.display().to_string(),
            export_mode.label().to_string(),
            segment.name.clone(),
            segment_output_path,
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

pub fn output_extension(path: &Path) -> String {
    path.extension()
        .and_then(|value| value.to_str())
        .filter(|value| !value.is_empty())
        .unwrap_or("mov")
        .to_string()
}

pub fn temp_segment_path(temp_dir: &Path, index: usize, extension: &str) -> PathBuf {
    temp_dir.join(format!("segment-{index:04}.{extension}"))
}
