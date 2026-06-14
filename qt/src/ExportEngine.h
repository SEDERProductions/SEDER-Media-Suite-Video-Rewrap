#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <functional>
#include <atomic>

class SegmentTableModel;

class ExportEngine : public QObject
{
    Q_OBJECT

public:
    explicit ExportEngine(QObject *parent = nullptr);

    bool busy() const { return m_busy; }
    double progress() const { return m_progress; }
    QString exportMode() const { return m_exportMode; }
    bool overwriteApprovedForSession() const { return m_overwriteApprovedForSession; }

    void setExportMode(const QString &mode);

    bool ensureCanExport(
        bool ffmpegReady,
        bool ffprobeReady,
        const QString &sourcePath,
        const QString &outputPath,
        SegmentTableModel *segments,
        const QJsonArray &keyframesJson,
        bool busy);

    void startExport(
        const QString &sourcePath,
        const QString &outputPath,
        const QJsonObject &metadataJson,
        SegmentTableModel *segments,
        const QJsonArray &keyframesJson,
        bool overwriteApproved);

    void cancel();
    void resetOverwriteSession();

    // Testing hooks
    void setToolsReadyForTesting(bool ffmpegReady, bool ffprobeReady);
    void setOverwriteDecisionProviderForTesting(const std::function<bool(const QString &)> &provider);
    bool ensureCanExportForTesting(
        bool ffmpegReady,
        bool ffprobeReady,
        const QString &sourcePath,
        const QString &outputPath,
        SegmentTableModel *segments,
        const QJsonArray &keyframesJson,
        bool busy);

signals:
    void busyChanged(bool busy);
    void progressChanged(double progress);
    void exportModeChanged(QString mode);
    void logMessage(QString message);
    void errorReport(QString details);
    // Terminal outcome of an export job (not emitted for cancels).
    void finished(bool ok, QString message);

private:
    bool requestOverwriteApproval(const QString &outputPath);
    void setBusy(bool busy);
    void setProgress(double progress);

    bool m_busy = false;
    double m_progress = 0.0;
    QString m_exportMode = QStringLiteral("concat_single");
    std::atomic_bool m_cancelExport = false;
    bool m_overwriteApprovedForSession = false;
    std::function<bool(const QString &)> m_overwriteDecisionProvider;
};
