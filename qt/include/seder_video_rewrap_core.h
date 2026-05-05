#pragma once

#ifdef __cplusplus
extern "C" {
#endif

char *svr_parse_timecode(const char *value);
char *svr_format_timecode(long long milliseconds);
char *svr_ffprobe_metadata_command(const char *source);
char *svr_ffprobe_keyframe_command(const char *source);
char *svr_ffplay_preview_command(const char *source, long long start_ms);
char *svr_ffmpeg_segment_command(const char *source, const char *segment_json, const char *keyframes_json, const char *output);
char *svr_ffmpeg_concat_command(const char *list_file, const char *output);
char *svr_parse_probe_result(
    const char *source,
    unsigned long long file_size,
    const char *metadata_output,
    const char *keyframe_output);
char *svr_nearest_keyframe(const char *keyframes_json, long long requested_ms);
char *svr_validate_segments(const char *segments_json, const char *keyframes_json);
char *svr_rewrap_preflight(const char *metadata_json, const char *output, const char *segments_json, const char *keyframes_json);
char *svr_export_plan(
    const char *source,
    const char *output,
    const char *temp_root,
    const char *segments_json,
    const char *keyframes_json);
char *svr_project_json(const char *source, const char *output, const char *segments_json);
char *svr_parse_project_json(const char *project_json);
char *svr_rewrap_report_txt(const char *source, const char *output, const char *segments_json);
char *svr_rewrap_report_csv(const char *source, const char *output, const char *segments_json);
void svr_free_string(char *ptr);

#ifdef __cplusplus
}
#endif
