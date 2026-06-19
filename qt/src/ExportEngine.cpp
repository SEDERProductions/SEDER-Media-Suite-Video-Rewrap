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
#include <QPointer>
#include <QThread>
#include <QSharedPointer>

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

void ExportEngine::postLogMessage(const QString &message)
{
    // Safe to call from any thread; the signal is connected with
    // AutoConnection which becomes QueuedConnection across threads.
    emit logMessage(message);
}

void ExportEngine::postErrorReport(const QString &details)
{
    emit errorReport(details);
}

void ExportEngine::postFinishedSignal(bool ok, const QString &message)
{
    emit finished(ok, message);
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

    // Snapshot the plan on the heap so the worker thread doesn't capture
    // a giant QJsonObject by value. QPointer keeps the worker safe if
    // ExportEngine is destroyed (e.g. app shutdown) before the export
    // finishes.
    const QSharedPointer<const QJsonObject> planPtr(
        new QJsonObject(plan), [](const QJsonObject *p) { delete p; });
    const int segmentCount = plan.value("segments").toArray().size();
    const int exportModeIsConcat = m_exportMode == "concat_single" ? 1 : 0;

    QPointer<ExportEngine> self(this);
    QThread *worker = QThread::create([self, outputPath, tempRoot, planPtr, segmentCount, exportModeIsConcat] {
        TempDirGuard guard { tempRoot };

        // m_busy / m_progress are non-atomic, so any state mutation from
        // the worker must be queued back to the GUI thread. Signal
        // emission is thread-safe via auto-connection (becomes
        // QueuedConnection across threads).
        auto queueToMain = [self](std::function<void(ExportEngine *)> fn) {
            if (!self) return;
            QMetaObject::invokeMethod(self.data(), [self, fn] {
                if (self) fn(self.data());
            }, Qt::QueuedConnection);
        };
        auto postLog = [self](const QString &message) {
            if (self) self->postLogMessage(message);
        };
        auto postError = [self, queueToMain, postLog](const QString &userMessage, const QString &details) {
            if (self) self->postErrorReport(details);
            postLog(userMessage);
            queueToMain([](ExportEngine *e) { e->setBusy(false); e->setProgress(0.0); });
        };

        if (!QDir().mkpath(tempRoot)) {
            postError(QStringLiteral("Unable to create temporary export folder: %1").arg(displayPath(tempRoot)),
                      QStringLiteral("Failed to create temp dir: %1").arg(tempRoot));
            return;
        }

        const QJsonArray plannedSegments = planPtr->value("segments").toArray();
        for (int i = 0; i < plannedSegments.size(); ++i) {
            if (self && self->m_cancelExport) {
                postLog("Export cancelled.");
                queueToMain([](ExportEngine *e) { e->setBusy(false); e->setProgress(0.0); });
                return;
            }
            const QJsonObject item = plannedSegments.at(i).toObject();
            const QString path = item.value("path").toString();
            const QJsonObject cmdObj = item.value("command").toObject();
            const RustBridge::Command command = RustBridge::commandFromJson(cmdObj);
            const int idx = i;
            const int total = segmentCount;
            postLog(QStringLiteral("Exporting segment %1 of %2...").arg(idx + 1).arg(total));
            queueToMain([idx, total](ExportEngine *e) {
                e->setProgress(static_cast<double>(idx) / static_cast<double>(total + 1));
            });
            std::atomic_bool *cancel = self ? &self->m_cancelExport : nullptr;
            const ProcessResult result = ProcessUtil::runCommand(command.program, command.args, cancel);
            if (!result.ok) {
                const int segmentIndex = i + 1;
                const QString details = QStringLiteral(
                    "Segment %1 of %2 failed\nOutput path: %3\nFFmpeg exit code: %4\n\n--- ffmpeg stderr ---\n%5")
                    .arg(segmentIndex)
                    .arg(segmentCount)
                    .arg(displayPath(path))
                    .arg(result.exitCode)
                    .arg(result.stderrText);
                const QString summary = QStringLiteral("Segment export failed for %1\n%2")
                    .arg(displayPath(path), result.stderrText);
                postError(summary, details);
                if (self) {
                    self->postFinishedSignal(false, QStringLiteral("Segment %1 export failed").arg(segmentIndex));
                }
                return;
            }
        }

        ProcessResult result{ true, "", "", 0 };
        if (exportModeIsConcat) {
            QFile listFile(planPtr->value("listPath").toString());
            if (!listFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                postError(QStringLiteral("Unable to write concat list: %1").arg(listFile.errorString()),
                          QStringLiteral("Concat list open failed: %1").arg(listFile.errorString()));
                return;
            }
            const QByteArray listBytes = planPtr->value("listText").toString().toUtf8();
            if (listFile.write(listBytes) != listBytes.size()) {
                const QString error = listFile.errorString();
                listFile.close();
                postError(QStringLiteral("Unable to write concat list: %1").arg(error),
                          QStringLiteral("Concat list write failed: %1").arg(error));
                return;
            }
            listFile.flush();
            if (listFile.error() != QFileDevice::NoError) {
                const QString error = listFile.errorString();
                listFile.close();
                postError(QStringLiteral("Unable to flush concat list: %1").arg(error),
                          QStringLiteral("Concat list flush failed: %1").arg(error));
                return;
            }
            listFile.close();
            postLog("Concatenating enabled segments...");
            queueToMain([](ExportEngine *e) { e->setProgress(0.9); });
            const QJsonObject concatCmdObj = planPtr->value("concatCommand").toObject();
            const RustBridge::Command concat = RustBridge::commandFromJson(concatCmdObj);
            std::atomic_bool *cancel = self ? &self->m_cancelExport : nullptr;
            result = ProcessUtil::runCommand(concat.program, concat.args, cancel);
        }

        if (self) {
            const ProcessResult finalResult = result;
            queueToMain([finalResult, outputPath](ExportEngine *e) {
                e->setBusy(false);
                if (finalResult.ok) {
                    e->setProgress(1.0);
                    e->postLogMessage(QStringLiteral("Export complete: %1").arg(displayPath(outputPath)));
                    e->postFinishedSignal(true, QStringLiteral("Export complete"));
                } else {
                    e->setProgress(0.0);
                    const QString details = QStringLiteral(
                        "Concat step failed\nOutput path: %1\nFFmpeg exit code: %2\n\n--- ffmpeg stderr ---\n%3")
                        .arg(displayPath(outputPath))
                        .arg(finalResult.exitCode)
                        .arg(finalResult.stderrText);
                    e->postErrorReport(details);
                    e->postLogMessage(QStringLiteral("Export failed. No re-encode fallback was attempted.\n%1")
                        .arg(finalResult.stderrText));
                    e->postFinishedSignal(false, QStringLiteral("Export failed"));
                }
            });
        }
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
