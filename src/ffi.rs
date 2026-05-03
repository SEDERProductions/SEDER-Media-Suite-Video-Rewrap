use crate::media_core::video_rewrap::{
    ffmpeg_concat_command, ffmpeg_segment_command, ffplay_preview_command,
    ffprobe_keyframe_command, ffprobe_metadata_command, format_ms, nearest_keyframe,
    parse_ffprobe_keyframes, parse_ffprobe_metadata, parse_timecode_to_ms, rewrap_report_csv,
    rewrap_report_txt, temp_segment_path, validate_segments, RewrapProject, RewrapSegment,
    CURRENT_PROJECT_VERSION,
};
use anyhow::{Context, Result};
use serde_json::{json, Value};
use std::ffi::{CStr, CString};
use std::os::raw::c_char;
use std::path::{Path, PathBuf};

fn input(ptr: *const c_char, name: &str) -> Result<String> {
    if ptr.is_null() {
        anyhow::bail!("{name} is null");
    }
    let value = unsafe { CStr::from_ptr(ptr) }
        .to_str()
        .with_context(|| format!("{name} is not valid UTF-8"))?;
    Ok(value.to_string())
}

fn path_string(path: &Path) -> String {
    path.to_string_lossy().to_string()
}

fn parse_segments(value: &str) -> Result<Vec<RewrapSegment>> {
    serde_json::from_str(value).context("Invalid segment JSON")
}

fn parse_keyframes(value: &str) -> Result<Vec<i64>> {
    serde_json::from_str(value).context("Invalid keyframe JSON")
}

fn concat_escape(path: &Path) -> String {
    path.to_string_lossy().replace('\'', "'\\''")
}

fn concat_list(paths: &[PathBuf]) -> String {
    paths
        .iter()
        .map(|path| format!("file '{}'\n", concat_escape(path)))
        .collect::<String>()
}

fn output_extension(path: &Path) -> String {
    path.extension()
        .and_then(|value| value.to_str())
        .filter(|value| !value.is_empty())
        .unwrap_or("mov")
        .to_string()
}

fn response(result: Result<Value>) -> *mut c_char {
    let value = match result {
        Ok(payload) => {
            let mut object = serde_json::Map::new();
            object.insert("ok".to_string(), Value::Bool(true));
            if let Value::Object(payload) = payload {
                object.extend(payload);
            } else {
                object.insert("value".to_string(), payload);
            }
            Value::Object(object)
        }
        Err(err) => json!({
            "ok": false,
            "error": err.to_string(),
        }),
    };
    let text = serde_json::to_string(&value).unwrap_or_else(|_| {
        "{\"ok\":false,\"error\":\"Unable to serialize response\"}".to_string()
    });
    CString::new(text.replace('\0', "\\u0000"))
        .expect("response JSON should not contain nul bytes")
        .into_raw()
}

#[no_mangle]
#[allow(clippy::not_unsafe_ptr_arg_deref)]
pub extern "C" fn svr_free_string(ptr: *mut c_char) {
    if ptr.is_null() {
        return;
    }
    unsafe {
        let _ = CString::from_raw(ptr);
    }
}

#[no_mangle]
pub extern "C" fn svr_parse_timecode(value: *const c_char) -> *mut c_char {
    response((|| {
        let value = input(value, "timecode")?;
        let milliseconds = parse_timecode_to_ms(&value)?;
        Ok(json!({ "milliseconds": milliseconds }))
    })())
}

#[no_mangle]
pub extern "C" fn svr_format_timecode(milliseconds: i64) -> *mut c_char {
    response(Ok(json!({ "timecode": format_ms(milliseconds) })))
}

#[no_mangle]
pub extern "C" fn svr_ffprobe_metadata_command(source: *const c_char) -> *mut c_char {
    response((|| {
        let source = input(source, "source")?;
        Ok(json!({ "command": ffprobe_metadata_command(Path::new(&source)) }))
    })())
}

#[no_mangle]
pub extern "C" fn svr_ffprobe_keyframe_command(source: *const c_char) -> *mut c_char {
    response((|| {
        let source = input(source, "source")?;
        Ok(json!({ "command": ffprobe_keyframe_command(Path::new(&source)) }))
    })())
}

#[no_mangle]
pub extern "C" fn svr_ffplay_preview_command(source: *const c_char, start_ms: i64) -> *mut c_char {
    response((|| {
        let source = input(source, "source")?;
        Ok(json!({ "command": ffplay_preview_command(Path::new(&source), start_ms) }))
    })())
}

#[no_mangle]
pub extern "C" fn svr_ffmpeg_segment_command(
    source: *const c_char,
    segment_json: *const c_char,
    keyframes_json: *const c_char,
    output: *const c_char,
) -> *mut c_char {
    response((|| {
        let source = input(source, "source")?;
        let output = input(output, "output")?;
        let segment_json = input(segment_json, "segment_json")?;
        let keyframes_json = input(keyframes_json, "keyframes_json")?;
        let segment: RewrapSegment =
            serde_json::from_str(&segment_json).context("Invalid segment JSON")?;
        let keyframes = parse_keyframes(&keyframes_json)?;
        Ok(json!({
            "command": ffmpeg_segment_command(Path::new(&source), &segment, &keyframes, Path::new(&output)),
        }))
    })())
}

#[no_mangle]
pub extern "C" fn svr_ffmpeg_concat_command(
    list_file: *const c_char,
    output: *const c_char,
) -> *mut c_char {
    response((|| {
        let list_file = input(list_file, "list_file")?;
        let output = input(output, "output")?;
        Ok(json!({
            "command": ffmpeg_concat_command(Path::new(&list_file), Path::new(&output)),
        }))
    })())
}

#[no_mangle]
pub extern "C" fn svr_parse_probe_result(
    source: *const c_char,
    file_size: u64,
    metadata_output: *const c_char,
    keyframe_output: *const c_char,
) -> *mut c_char {
    response((|| {
        let source = input(source, "source")?;
        let metadata_output = input(metadata_output, "metadata_output")?;
        let keyframe_output = input(keyframe_output, "keyframe_output")?;
        let metadata = parse_ffprobe_metadata(&metadata_output, Path::new(&source), file_size);
        let keyframes = parse_ffprobe_keyframes(&keyframe_output);
        if keyframes.is_empty() {
            anyhow::bail!("No keyframes were detected in this file.");
        }
        Ok(json!({
            "metadata": metadata,
            "keyframes": keyframes,
        }))
    })())
}

#[no_mangle]
pub extern "C" fn svr_nearest_keyframe(
    keyframes_json: *const c_char,
    requested_ms: i64,
) -> *mut c_char {
    response((|| {
        let keyframes_json = input(keyframes_json, "keyframes_json")?;
        let keyframes = parse_keyframes(&keyframes_json)?;
        let snap = nearest_keyframe(&keyframes, requested_ms)
            .context("No keyframes are available for snapping")?;
        Ok(json!({ "snap": snap }))
    })())
}

#[no_mangle]
pub extern "C" fn svr_validate_segments(
    segments_json: *const c_char,
    keyframes_json: *const c_char,
) -> *mut c_char {
    response((|| {
        let segments_json = input(segments_json, "segments_json")?;
        let keyframes_json = input(keyframes_json, "keyframes_json")?;
        let segments = parse_segments(&segments_json)?;
        let keyframes = parse_keyframes(&keyframes_json)?;
        validate_segments(&segments, &keyframes)?;
        let total_enabled_duration_ms = segments
            .iter()
            .filter(|segment| segment.enabled)
            .map(|segment| (segment.out_ms - segment.in_ms).max(0))
            .sum::<i64>();
        Ok(json!({
            "totalEnabledDurationMs": total_enabled_duration_ms,
        }))
    })())
}

#[no_mangle]
pub extern "C" fn svr_export_plan(
    source: *const c_char,
    output: *const c_char,
    temp_root: *const c_char,
    segments_json: *const c_char,
    keyframes_json: *const c_char,
) -> *mut c_char {
    response((|| {
        let source = input(source, "source")?;
        let output = input(output, "output")?;
        let temp_root = input(temp_root, "temp_root")?;
        let segments_json = input(segments_json, "segments_json")?;
        let keyframes_json = input(keyframes_json, "keyframes_json")?;
        let segments = parse_segments(&segments_json)?;
        let keyframes = parse_keyframes(&keyframes_json)?;
        validate_segments(&segments, &keyframes)?;
        let enabled = segments
            .iter()
            .filter(|segment| segment.enabled)
            .collect::<Vec<_>>();
        if enabled.is_empty() {
            anyhow::bail!("No enabled segments to export.");
        }

        let source = Path::new(&source);
        let output = Path::new(&output);
        let temp_root = PathBuf::from(temp_root);
        let extension = output_extension(output);
        let mut segment_paths = Vec::with_capacity(enabled.len());
        let mut planned_segments = Vec::with_capacity(enabled.len());
        for (index, segment) in enabled.iter().enumerate() {
            let segment_path = temp_segment_path(&temp_root, index + 1, &extension);
            let command = ffmpeg_segment_command(source, segment, &keyframes, &segment_path);
            planned_segments.push(json!({
                "segment": segment,
                "path": path_string(&segment_path),
                "command": command,
            }));
            segment_paths.push(segment_path);
        }
        let list_path = temp_root.join("concat.txt");
        Ok(json!({
            "segments": planned_segments,
            "listPath": path_string(&list_path),
            "listText": concat_list(&segment_paths),
            "concatCommand": ffmpeg_concat_command(&list_path, output),
        }))
    })())
}

#[no_mangle]
pub extern "C" fn svr_project_json(
    source: *const c_char,
    output: *const c_char,
    segments_json: *const c_char,
) -> *mut c_char {
    response((|| {
        let source = input(source, "source")?;
        let output = input(output, "output")?;
        let segments_json = input(segments_json, "segments_json")?;
        let project = RewrapProject {
            version: CURRENT_PROJECT_VERSION,
            source_file: source,
            output_file: output,
            segments: parse_segments(&segments_json)?,
        };
        Ok(json!({
            "projectJson": serde_json::to_string_pretty(&project)?,
        }))
    })())
}

#[no_mangle]
pub extern "C" fn svr_parse_project_json(project_json: *const c_char) -> *mut c_char {
    response((|| {
        let project_json = input(project_json, "project_json")?;
        let project: RewrapProject =
            serde_json::from_str(&project_json).context("Invalid project JSON")?;
        Ok(json!({ "project": project }))
    })())
}

#[no_mangle]
pub extern "C" fn svr_rewrap_report_txt(
    source: *const c_char,
    output: *const c_char,
    segments_json: *const c_char,
) -> *mut c_char {
    response((|| {
        let source = input(source, "source")?;
        let output = input(output, "output")?;
        let segments_json = input(segments_json, "segments_json")?;
        let segments = parse_segments(&segments_json)?;
        Ok(json!({
            "report": rewrap_report_txt(Path::new(&source), Path::new(&output), &segments),
        }))
    })())
}

#[no_mangle]
pub extern "C" fn svr_rewrap_report_csv(
    source: *const c_char,
    output: *const c_char,
    segments_json: *const c_char,
) -> *mut c_char {
    response((|| {
        let source = input(source, "source")?;
        let output = input(output, "output")?;
        let segments_json = input(segments_json, "segments_json")?;
        let segments = parse_segments(&segments_json)?;
        Ok(json!({
            "report": rewrap_report_csv(Path::new(&source), Path::new(&output), &segments),
        }))
    })())
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::ffi::CString;

    fn call(ptr: *mut c_char) -> Value {
        let text = unsafe { CStr::from_ptr(ptr) }.to_string_lossy().to_string();
        svr_free_string(ptr);
        serde_json::from_str(&text).unwrap()
    }

    #[test]
    fn ffi_formats_success_and_error_responses() {
        let valid = CString::new("00:00:02.500").unwrap();
        let parsed = call(svr_parse_timecode(valid.as_ptr()));
        assert_eq!(parsed["ok"], true);
        assert_eq!(parsed["milliseconds"], 2_500);

        let invalid = CString::new("bad").unwrap();
        let parsed = call(svr_parse_timecode(invalid.as_ptr()));
        assert_eq!(parsed["ok"], false);
        assert!(parsed["error"]
            .as_str()
            .unwrap()
            .contains("Invalid seconds"));
    }

    #[test]
    fn ffi_returns_commands_as_json() {
        let source = CString::new("/tmp/source.mov").unwrap();
        let parsed = call(svr_ffprobe_metadata_command(source.as_ptr()));
        assert_eq!(parsed["ok"], true);
        assert_eq!(parsed["command"]["program"], "ffprobe");
        assert!(parsed["command"]["args"]
            .as_array()
            .unwrap()
            .iter()
            .any(|arg| arg == "/tmp/source.mov"));
    }

    #[test]
    fn concat_escape_handles_single_quotes() {
        // ffmpeg concat demuxer requires single-quote escaping inside
        // single-quoted file paths: ' → '\''
        let escaped = concat_escape(Path::new("it's.mov"));
        assert_eq!(escaped, "it'\\''s.mov");
        let listed = concat_list(&[std::path::PathBuf::from("it's.mov")]);
        assert_eq!(listed, "file 'it'\\''s.mov'\n");
    }

    #[test]
    fn ffi_builds_export_plan() {
        let source = CString::new("/tmp/source.mov").unwrap();
        let output = CString::new("/tmp/out.mov").unwrap();
        let temp = CString::new("/tmp/seder-video-rewrap").unwrap();
        let segments =
            CString::new(r#"[{"name":"A","in_ms":0,"out_ms":1000,"notes":"","enabled":true}]"#)
                .unwrap();
        let keyframes = CString::new("[0,1000]").unwrap();
        let parsed = call(svr_export_plan(
            source.as_ptr(),
            output.as_ptr(),
            temp.as_ptr(),
            segments.as_ptr(),
            keyframes.as_ptr(),
        ));
        assert_eq!(parsed["ok"], true);
        assert_eq!(parsed["segments"].as_array().unwrap().len(), 1);
        assert!(parsed["listText"]
            .as_str()
            .unwrap()
            .contains("segment-0001.mov"));
        assert_eq!(parsed["concatCommand"]["program"], "ffmpeg");
    }
}
