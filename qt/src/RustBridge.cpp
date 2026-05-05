#include "RustBridge.h"

#include "seder_video_rewrap_core.h"

#include <QJsonDocument>

namespace {
QByteArray utf8(const QString &value)
{
    return value.toUtf8();
}
}

QByteArray RustBridge::jsonBytes(const QJsonArray &array)
{
    return QJsonDocument(array).toJson(QJsonDocument::Compact);
}

static QByteArray jsonObjectBytes(const QJsonObject &object)
{
    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

QJsonObject RustBridge::decode(char *raw)
{
    if (!raw) {
        return QJsonObject {
            { "ok", false },
            { "error", "Rust core returned a null response" },
        };
    }

    const QByteArray payload(raw);
    svr_free_string(raw);

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return QJsonObject {
            { "ok", false },
            { "error", QStringLiteral("Rust core returned invalid JSON: %1").arg(error.errorString()) },
        };
    }
    return document.object();
}

QJsonObject RustBridge::parseTimecode(const QString &value)
{
    const QByteArray v = utf8(value);
    return decode(svr_parse_timecode(v.constData()));
}

QJsonObject RustBridge::formatTimecode(qint64 milliseconds)
{
    return decode(svr_format_timecode(milliseconds));
}

QJsonObject RustBridge::ffprobeMetadataCommand(const QString &source)
{
    const QByteArray s = utf8(source);
    return decode(svr_ffprobe_metadata_command(s.constData()));
}

QJsonObject RustBridge::ffprobeKeyframeCommand(const QString &source)
{
    const QByteArray s = utf8(source);
    return decode(svr_ffprobe_keyframe_command(s.constData()));
}

QJsonObject RustBridge::ffplayPreviewCommand(const QString &source, qint64 startMs)
{
    const QByteArray s = utf8(source);
    return decode(svr_ffplay_preview_command(s.constData(), startMs));
}

QJsonObject RustBridge::parseProbeResult(
    const QString &source,
    quint64 fileSize,
    const QString &metadataOutput,
    const QString &keyframeOutput)
{
    const QByteArray s = utf8(source);
    const QByteArray m = utf8(metadataOutput);
    const QByteArray k = utf8(keyframeOutput);
    return decode(svr_parse_probe_result(
        s.constData(),
        fileSize,
        m.constData(),
        k.constData()));
}

QJsonObject RustBridge::nearestKeyframe(const QJsonArray &keyframes, qint64 requestedMs)
{
    const QByteArray keys = jsonBytes(keyframes);
    return decode(svr_nearest_keyframe(keys.constData(), requestedMs));
}

QJsonObject RustBridge::validateSegments(const QJsonArray &segments, const QJsonArray &keyframes)
{
    const QByteArray s = jsonBytes(segments);
    const QByteArray k = jsonBytes(keyframes);
    return decode(svr_validate_segments(s.constData(), k.constData()));
}

QJsonObject RustBridge::rewrapPreflight(
    const QJsonObject &metadata,
    const QString &output,
    const QJsonArray &segments,
    const QJsonArray &keyframes)
{
    const QByteArray m = jsonObjectBytes(metadata);
    const QByteArray out = utf8(output);
    const QByteArray s = jsonBytes(segments);
    const QByteArray k = jsonBytes(keyframes);
    return decode(svr_rewrap_preflight(m.constData(), out.constData(), s.constData(), k.constData()));
}

QJsonObject RustBridge::exportPlan(
    const QString &source,
    const QString &output,
    const QString &tempRoot,
    const QJsonArray &segments,
    const QJsonArray &keyframes)
{
    const QByteArray src = utf8(source);
    const QByteArray out = utf8(output);
    const QByteArray tmp = utf8(tempRoot);
    const QByteArray s = jsonBytes(segments);
    const QByteArray k = jsonBytes(keyframes);
    return decode(svr_export_plan(
        src.constData(),
        out.constData(),
        tmp.constData(),
        s.constData(),
        k.constData()));
}

QJsonObject RustBridge::projectJson(
    const QString &source,
    const QString &output,
    const QJsonArray &segments)
{
    const QByteArray src = utf8(source);
    const QByteArray out = utf8(output);
    const QByteArray s = jsonBytes(segments);
    return decode(svr_project_json(src.constData(), out.constData(), s.constData()));
}

QJsonObject RustBridge::parseProjectJson(const QString &projectJson)
{
    const QByteArray p = utf8(projectJson);
    return decode(svr_parse_project_json(p.constData()));
}

QJsonObject RustBridge::rewrapReportTxt(
    const QString &source,
    const QString &output,
    const QJsonArray &segments)
{
    const QByteArray src = utf8(source);
    const QByteArray out = utf8(output);
    const QByteArray s = jsonBytes(segments);
    return decode(svr_rewrap_report_txt(src.constData(), out.constData(), s.constData()));
}

QJsonObject RustBridge::rewrapReportCsv(
    const QString &source,
    const QString &output,
    const QJsonArray &segments)
{
    const QByteArray src = utf8(source);
    const QByteArray out = utf8(output);
    const QByteArray s = jsonBytes(segments);
    return decode(svr_rewrap_report_csv(src.constData(), out.constData(), s.constData()));
}

RustBridge::Command RustBridge::commandFromJson(const QJsonObject &object)
{
    Command command;
    command.program = object.value("program").toString();
    const QJsonArray args = object.value("args").toArray();
    for (const QJsonValue &arg : args) {
        command.args.push_back(arg.toString());
    }
    return command;
}
