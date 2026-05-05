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
        const QJsonArray &keyframes);
    static QJsonObject projectJson(
        const QString &source,
        const QString &output,
        const QJsonArray &segments);
    static QJsonObject parseProjectJson(const QString &projectJson);
    static QJsonObject rewrapReportTxt(
        const QString &source,
        const QString &output,
        const QJsonArray &segments);
    static QJsonObject rewrapReportCsv(
        const QString &source,
        const QString &output,
        const QJsonArray &segments);
    static Command commandFromJson(const QJsonObject &object);

private:
    using FfiCall = char *(*)();

    static QByteArray jsonBytes(const QJsonArray &array);
    static QJsonObject decode(char *raw);
};
