#include "ExportEngine.h"

#include "ProcessUtil.h"
#include "RustBridge.h"
#include "SegmentTableModel.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QThread>

namespace {
struct TempDirGuard {
    QString path;
    ~TempDirGuard() {
        if (!path.isEmpty()) {
            QDir(path).removeRecursively();
        }
    }
};

QString displayPath(const QString &path)
{
    return QDir::toNativeSeparators(path);
}
}

ExportEngine::ExportEngine(QObject *parent)
    : QObject(parent)
{
}

void ExportEngine::setExportMode(const QString &mode)
{
    const QString normalized = mode == "separate_files" ? QStringLiteral("separate_files") : QStringLiteral("concat_single");
    if (m_exportMode == normalized) return;
    m_exportMode = normalized;
    emit exportModeChanged(m_exportMode);
}

void ExportEngine::setBusy(bool busy)
{
    if (m_busy == busy) return;
    m_busy = busy;
    emit busyChanged(m_busy);
}

void ExportEngine::setProgress(double progress)
{
    const double clamped = std::clamp(progress, 0.0, 1.0);
    if (m_progress == clamped) return;
    m_progress = clamped;
    emit progressChanged(m_progress);
}

bool ExportEngine::ensureCanExport(
    bool ffmpegReady,
    bool ffprobeReady,
    const QString &sourcePath,
    const QString &outputPath,
    SegmentTableModel *segments,
    const QJsonArray &keyframesJson,
    bool busy)
{
    if (busy) {
        emit logMessage("Another operation is already running.");
        return false;
    }
    if (!ffmpegReady || !ffprobeReady) {
        emit logMessage("FFmpeg and/or FFprobe are missing.");
        return false;
    }
    if (sourcePath.isEmpty()) {
        emit logMessage("Choose a source video first.");
        return false;
    }
    if (outputPath.isEmpty()) {
        emit logMessage("Choose an output file first.");
        return false;
    }
    if (QFileInfo::exists(outputPath)) {
        if (!requestOverwriteApproval(outputPath)) {
            m_overwriteApprovedForSession = false;
            emit logMessage(QStringLiteral("Export cancelled: overwrite declined for %1").arg(displayPath(outputPath)));
            return false;
        }
        m_overwriteApprovedForSession = true;
        emit logMessage(QStringLiteral("Overwrite approved for existing output: %1").arg(displayPath(outputPath)));
    } else {
        m_overwriteApprovedForSession = false;
    }
    if (!segments || segments->rowCount() == 0) {
        emit logMessage("Add at least one segment before exporting.");
        return false;
    }
    const QJsonObject validated = RustBridge::validateSegments(segments->toJsonArray(), keyframesJson);
    if (!validated.value("ok").toBool()) {
        emit logMessage(validated.value("error").toString());
        return false;
    }
    return true;
}

bool ExportEngine::requestOverwriteApproval(const QString &outputPath)
{
    if (m_overwriteDecisionProvider) {
        return m_overwriteDecisionProvider(outputPath);
    }

    const QMessageBox::StandardButton reply = QMessageBox::question(
        nullptr,
        QStringLiteral("Overwrite Output File"),
        QStringLiteral("The output file already exists:\n%1\n\nDo you want to overwrite it?").arg(displayPath(outputPath)),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    return reply == QMessageBox::Yes;
}

void ExportEngine::startExport(
    const QString &sourcePath,
    const QString &outputPath,
    const QJsonObject &metadataJson,
    SegmentTableModel *segments,
    const QJsonArray &keyframesJson,
    bool overwriteApproved)
{
    m_overwriteApprovedForSession = overwriteApproved;

    const QJsonObject preflight = RustBridge::rewrapPreflight(metadataJson, outputPath, segments->toJsonArray(), keyframesJson);
    if (!preflight.value("ok").toBool()) {
        emit logMessage(preflight.value("error").toString());
        return;
    }
    emit logMessage("Preflight passed. No re-encode fallback is available; stream-copy export will proceed.");

    const QString tempRoot = QDir::temp().filePath(QStringLiteral("seder-video-rewrap-%1-%2")
        .arg(QCoreApplication::applicationPid())
        .arg(QDateTime::currentMSecsSinceEpoch()));
    const QJsonObject plan = RustBridge::exportPlan(
        sourcePath,
        outputPath,
        tempRoot,
        segments->toJsonArray(),
        keyframesJson,
        m_exportMode);
    if (!plan.value("ok").toBool()) {
        emit logMessage(plan.value("error").toString());
        return;
    }

    m_cancelExport = false;
    setBusy(true);
    setProgress(0.05);
    emit logMessage("Exporting with FFmpeg stream copy only...");

    QThread *worker = QThread::create([this, sourcePath, outputPath, tempRoot, plan] {
        TempDirGuard guard { tempRoot };

        if (!QDir().mkpath(tempRoot)) {
            QMetaObject::invokeMethod(this, [this, tempRoot] {
                setBusy(false);
                setProgress(0.0);
                emit logMessage(QStringLiteral("Unable to create temporary export folder: %1")
                    .arg(displayPath(tempRoot)));
            }, Qt::QueuedConnection);
            return;
        }

        const QJsonArray plannedSegments = plan.value("segments").toArray();
        for (int i = 0; i < plannedSegments.size(); ++i) {
            if (m_cancelExport) {
                QMetaObject::invokeMethod(this, [this] {
                    setBusy(false);
                    setProgress(0.0);
                    emit logMessage("Export cancelled.");
                }, Qt::QueuedConnection);
                return;
            }
            const QJsonObject item = plannedSegments.at(i).toObject();
            const QString path = item.value("path").toString();
            const QJsonObject cmdObj = item.value("command").toObject();
            const RustBridge::Command command = RustBridge::commandFromJson(cmdObj);
            QMetaObject::invokeMethod(this, [this, i, plannedSegments] {
                setProgress(static_cast<double>(i) / static_cast<double>(plannedSegments.size() + 1));
                emit logMessage(QStringLiteral("Exporting segment %1 of %2...")
                    .arg(i + 1)
                    .arg(plannedSegments.size()));
            }, Qt::QueuedConnection);
            const ProcessResult result = ProcessUtil::runCommand(command.program, command.args, &m_cancelExport);
            if (!result.ok) {
                const int segmentIndex = i + 1;
                QMetaObject::invokeMethod(this, [this, path, result, segmentIndex, plannedSegments] {
                    setBusy(false);
                    setProgress(0.0);
                    const QString summary = QStringLiteral("Segment export failed for %1\n%2")
                        .arg(displayPath(path), result.stderrText);
                    const QString details = QStringLiteral(
                        "Segment %1 of %2 failed\nOutput path: %3\nFFmpeg exit code: %4\n\n--- ffmpeg stderr ---\n%5")
                        .arg(segmentIndex)
                        .arg(plannedSegments.size())
                        .arg(displayPath(path))
                        .arg(result.exitCode)
                        .arg(result.stderrText);
                    emit errorReport(details);
                    emit logMessage(summary);
                    emit finished(false, QStringLiteral("Segment %1 export failed").arg(segmentIndex));
                }, Qt::QueuedConnection);
                return;
            }
        }

        ProcessResult result{ true, "", "", 0 };
        if (m_exportMode == "concat_single") {
            QFile listFile(plan.value("listPath").toString());
            if (!listFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                const QString error = listFile.errorString();
                QMetaObject::invokeMethod(this, [this, error] {
                    setBusy(false);
                    setProgress(0.0);
                    emit logMessage(QStringLiteral("Unable to write concat list: %1").arg(error));
                }, Qt::QueuedConnection);
                return;
            }
            const QByteArray listBytes = plan.value("listText").toString().toUtf8();
            if (listFile.write(listBytes) != listBytes.size()) {
                const QString error = listFile.errorString();
                listFile.close();
                QMetaObject::invokeMethod(this, [this, error] {
                    setBusy(false);
                    setProgress(0.0);
                    emit logMessage(QStringLiteral("Unable to write concat list: %1").arg(error));
                }, Qt::QueuedConnection);
                return;
            }
            listFile.flush();
            if (listFile.error() != QFileDevice::NoError) {
                const QString error = listFile.errorString();
                listFile.close();
                QMetaObject::invokeMethod(this, [this, error] {
                    setBusy(false);
                    setProgress(0.0);
                    emit logMessage(QStringLiteral("Unable to flush concat list: %1").arg(error));
                }, Qt::QueuedConnection);
                return;
            }
            listFile.close();
            QMetaObject::invokeMethod(this, [this] {
                setProgress(0.9);
                emit logMessage("Concatenating enabled segments...");
            }, Qt::QueuedConnection);
            const QJsonObject concatCmdObj = plan.value("concatCommand").toObject();
            const RustBridge::Command concat = RustBridge::commandFromJson(concatCmdObj);
            result = ProcessUtil::runCommand(concat.program, concat.args, &m_cancelExport);
        }

        QMetaObject::invokeMethod(this, [this, result, outputPath] {
            setBusy(false);
            if (result.ok) {
                setProgress(1.0);
                emit logMessage(QStringLiteral("Export complete: %1").arg(displayPath(outputPath)));
                emit finished(true, QStringLiteral("Export complete"));
            } else {
                setProgress(0.0);
                const QString details = QStringLiteral(
                    "Concat step failed\nOutput path: %1\nFFmpeg exit code: %2\n\n--- ffmpeg stderr ---\n%3")
                    .arg(displayPath(outputPath))
                    .arg(result.exitCode)
                    .arg(result.stderrText);
                emit errorReport(details);
                emit logMessage(QStringLiteral("Export failed. No re-encode fallback was attempted.\n%1")
                    .arg(result.stderrText));
                emit finished(false, QStringLiteral("Export failed"));
            }
        }, Qt::QueuedConnection);
    });
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

void ExportEngine::cancel()
{
    if (!m_busy) return;
    m_cancelExport = true;
    emit logMessage("Cancelling export...");
}

void ExportEngine::resetOverwriteSession()
{
    m_overwriteApprovedForSession = false;
}

void ExportEngine::setToolsReadyForTesting(bool ffmpegReady, bool ffprobeReady)
{
    Q_UNUSED(ffmpegReady);
    Q_UNUSED(ffprobeReady);
}

void ExportEngine::setOverwriteDecisionProviderForTesting(const std::function<bool(const QString &)> &provider)
{
    m_overwriteDecisionProvider = provider;
}

bool ExportEngine::ensureCanExportForTesting(
    bool ffmpegReady,
    bool ffprobeReady,
    const QString &sourcePath,
    const QString &outputPath,
    SegmentTableModel *segments,
    const QJsonArray &keyframesJson,
    bool busy)
{
    return ensureCanExport(ffmpegReady, ffprobeReady, sourcePath, outputPath, segments, keyframesJson, busy);
}
