#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>

class RustBridge
{
public:
    struct Command {
        QString program;
        QStringList args;
    };

    static QJsonObject parseTimecode(const QString &value);
    static QJsonObject formatTimecode(qint64 milliseconds);
    static QJsonObject ffprobeMetadataCommand(const QString &source);
    static QJsonObject ffprobeKeyframeCommand(const QString &source);
    static QJsonObject ffplayPreviewCommand(const QString &source, qint64 startMs);
    static QJsonObject ffmpegThumbnailCommand(const QString &source, qint64 timeMs, const QString &output);
    static QJsonObject parseProbeResult(
        const QString &source,
        quint64 fileSize,
        const QString &metadataOutput,
        const QString &keyframeOutput);
    static QJsonObject nearestKeyframe(const QJsonArray &keyframes, qint64 requestedMs);
    static QJsonObject validateSegments(const QJsonArray &segments, const QJsonArray &keyframes);
    static QJsonObject rewrapPreflight(const QJsonObject &metadata,
                                       const QString &output,
                                       const QJsonArray &segments,
                                       const QJsonArray &keyframes);
    static QJsonObject exportPlan(
        const QString &source,
        const QString &output,
        const QString &tempRoot,
        const QJsonArray &segments,
        const QJsonArray &keyframes,
        const QString &exportMode);
    static QJsonObject projectJson(
        const QString &source,
        const QString &output,
        const QJsonArray &segments);
    static QJsonObject parseProjectJson(const QString &projectJson);
    static QJsonObject rewrapReportTxt(
        const QString &source,
        const QString &output,
        const QJsonArray &segments,
        const QString &exportMode);
    static QJsonObject rewrapReportCsv(
        const QString &source,
        const QString &output,
        const QJsonArray &segments,
        const QString &exportMode);
    static QJsonObject ffmpegCompatibility(const QString &versionOutput);
    static QJsonObject evaluateUpdate(const QString &latestJson, const QString &currentVersion);
    static Command commandFromJson(const QJsonObject &object);

private:
    using FfiCallQStr1 = char *(*)(const char *);
    using FfiCallQStr2Json1 = char *(*)(const char *, const char *, const char *);
    using FfiCallJson1I64 = char *(*)(const char *, qint64);
    using FfiCallQStr1I64 = char *(*)(const char *, qint64);
    using FfiCallQStr1I64QStr1 = char *(*)(const char *, qint64, const char *);
    using FfiCallQStr1U64QStr2 = char *(*)(const char *, quint64, const char *, const char *);
    using FfiCallJson2 = char *(*)(const char *, const char *);
    using FfiCallQStr3Json2 = char *(*)(const char *, const char *, const char *, const char *, const char *);

    static QByteArray jsonBytes(const QJsonArray &array);
    static QJsonObject decode(char *raw);
    static QJsonObject invokeQStr1(FfiCallQStr1 call, const QString &value);
    static QJsonObject invokeQStrQStrJson(
        FfiCallQStr2Json1 call,
        const QString &first,
        const QString &second,
        const QJsonArray &array);
    static QJsonObject invokeJsonI64(FfiCallJson1I64 call, const QJsonArray &array, qint64 value);
    static QJsonObject invokeQStrI64(FfiCallQStr1I64 call, const QString &value, qint64 number);
    static QJsonObject invokeQStrI64QStr(
        FfiCallQStr1I64QStr1 call,
        const QString &first,
        qint64 number,
        const QString &second);
    static QJsonObject invokeQStrU64QStrQStr(
        FfiCallQStr1U64QStr2 call,
        const QString &first,
        quint64 number,
        const QString &second,
        const QString &third);
    static QJsonObject invokeJsonJson(FfiCallJson2 call, const QJsonArray &first, const QJsonArray &second);
    static QJsonObject invokeQStrQStrQStrJsonJson(
        FfiCallQStr3Json2 call,
        const QString &first,
        const QString &second,
        const QString &third,
        const QJsonArray &fourth,
        const QJsonArray &fifth);
};
