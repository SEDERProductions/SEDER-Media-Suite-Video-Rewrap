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

QJsonObject RustBridge::invokeQStr1(FfiCallQStr1 call, const QString &value)
{
    const QByteArray v = utf8(value);
    return decode(call(v.constData()));
}

QJsonObject RustBridge::invokeQStrQStrJson(
    FfiCallQStr2Json1 call,
    const QString &first,
    const QString &second,
    const QJsonArray &array)
{
    const QByteArray a = utf8(first);
    const QByteArray b = utf8(second);
    const QByteArray c = jsonBytes(array);
    return decode(call(a.constData(), b.constData(), c.constData()));
}

QJsonObject RustBridge::invokeJsonI64(FfiCallJson1I64 call, const QJsonArray &array, qint64 value)
{
    const QByteArray a = jsonBytes(array);
    return decode(call(a.constData(), value));
}

QJsonObject RustBridge::invokeQStrI64(FfiCallQStr1I64 call, const QString &value, qint64 number)
{
    const QByteArray v = utf8(value);
    return decode(call(v.constData(), number));
}

QJsonObject RustBridge::invokeQStrU64QStrQStr(
    FfiCallQStr1U64QStr2 call,
    const QString &first,
    quint64 number,
    const QString &second,
    const QString &third)
{
    const QByteArray a = utf8(first);
    const QByteArray b = utf8(second);
    const QByteArray c = utf8(third);
    return decode(call(a.constData(), number, b.constData(), c.constData()));
}

QJsonObject RustBridge::invokeJsonJson(FfiCallJson2 call, const QJsonArray &first, const QJsonArray &second)
{
    const QByteArray a = jsonBytes(first);
    const QByteArray b = jsonBytes(second);
    return decode(call(a.constData(), b.constData()));
}

QJsonObject RustBridge::invokeQStrQStrQStrJsonJson(
    FfiCallQStr3Json2 call,
    const QString &first,
    const QString &second,
    const QString &third,
    const QJsonArray &fourth,
    const QJsonArray &fifth)
{
    const QByteArray a = utf8(first);
    const QByteArray b = utf8(second);
    const QByteArray c = utf8(third);
    const QByteArray d = jsonBytes(fourth);
    const QByteArray e = jsonBytes(fifth);
    return decode(call(a.constData(), b.constData(), c.constData(), d.constData(), e.constData()));
}

QJsonObject RustBridge::parseTimecode(const QString &value)
{
    return invokeQStr1(svr_parse_timecode, value);
}

QJsonObject RustBridge::formatTimecode(qint64 milliseconds)
{
    return decode(svr_format_timecode(milliseconds));
}

QJsonObject RustBridge::ffprobeMetadataCommand(const QString &source)
{
    return invokeQStr1(svr_ffprobe_metadata_command, source);
}

QJsonObject RustBridge::ffprobeKeyframeCommand(const QString &source)
{
    return invokeQStr1(svr_ffprobe_keyframe_command, source);
}

QJsonObject RustBridge::ffplayPreviewCommand(const QString &source, qint64 startMs)
{
    return invokeQStrI64(svr_ffplay_preview_command, source, startMs);
}

QJsonObject RustBridge::parseProbeResult(
    const QString &source,
    quint64 fileSize,
    const QString &metadataOutput,
    const QString &keyframeOutput)
{
    return invokeQStrU64QStrQStr(
        svr_parse_probe_result,
        source,
        fileSize,
        metadataOutput,
        keyframeOutput);
}

QJsonObject RustBridge::nearestKeyframe(const QJsonArray &keyframes, qint64 requestedMs)
{
    return invokeJsonI64(svr_nearest_keyframe, keyframes, requestedMs);
}

QJsonObject RustBridge::validateSegments(const QJsonArray &segments, const QJsonArray &keyframes)
{
    return invokeJsonJson(svr_validate_segments, segments, keyframes);
}

QJsonObject RustBridge::exportPlan(
    const QString &source,
    const QString &output,
    const QString &tempRoot,
    const QJsonArray &segments,
    const QJsonArray &keyframes)
{
    return invokeQStrQStrQStrJsonJson(
        svr_export_plan,
        source,
        output,
        tempRoot,
        segments,
        keyframes);
}

QJsonObject RustBridge::projectJson(
    const QString &source,
    const QString &output,
    const QJsonArray &segments)
{
    return invokeQStrQStrJson(svr_project_json, source, output, segments);
}

QJsonObject RustBridge::parseProjectJson(const QString &projectJson)
{
    return invokeQStr1(svr_parse_project_json, projectJson);
}

QJsonObject RustBridge::rewrapReportTxt(
    const QString &source,
    const QString &output,
    const QJsonArray &segments)
{
    return invokeQStrQStrJson(svr_rewrap_report_txt, source, output, segments);
}

QJsonObject RustBridge::rewrapReportCsv(
    const QString &source,
    const QString &output,
    const QJsonArray &segments)
{
    return invokeQStrQStrJson(svr_rewrap_report_csv, source, output, segments);
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
