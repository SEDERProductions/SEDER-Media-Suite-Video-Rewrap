#include "AppController.h"

#include <QDateTime>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QStyleHints>
#include <QThread>
#include <algorithm>

AppController::AppController(SegmentTableModel *segments, QObject *parent)
    : QObject(parent)
    , m_segments(segments)
{
    QSettings settings;
    m_theme = settings.value("theme", "system").toString();
    setTheme(m_theme);
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, [this] {
        if (m_theme == "system") {
            setTheme("system");
        }
    });
    recheckToolsBackground();
}

SegmentTableModel *AppController::segmentModel() const { return m_segments; }
QString AppController::sourcePath() const { return m_sourcePath; }
QString AppController::outputPath() const { return m_outputPath; }
QString AppController::logText() const { return m_logText; }
bool AppController::busy() const { return m_busy; }
double AppController::progress() const { return m_progress; }
bool AppController::ffmpegReady() const { return m_ffmpegReady; }
bool AppController::ffprobeReady() const { return m_ffprobeReady; }
bool AppController::ffplayReady() const { return m_ffplayReady; }
QString AppController::mediaFilename() const { return m_mediaFilename; }
QString AppController::durationText() const { return m_durationText; }
QString AppController::resolutionText() const { return m_resolutionText; }
QString AppController::codecText() const { return m_codecText; }
QString AppController::sizeText() const { return m_sizeText; }
QString AppController::mediaSummary() const { return m_mediaSummary; }
int AppController::keyframeCount() const { return m_keyframes.size(); }
QString AppController::currentKeyframeText() const { return formatMs(currentKeyframeMs()); }
QString AppController::pendingInText() const { return m_hasPendingIn ? formatMs(m_pendingIn) : "-"; }
QString AppController::pendingOutText() const { return m_hasPendingOut ? formatMs(m_pendingOut) : "-"; }
QString AppController::totalDurationText() const { return m_totalDurationText; }
int AppController::selectedRow() const { return m_selectedRow; }
QString AppController::theme() const { return m_theme; }
bool AppController::darkMode() const { return m_darkMode; }

void AppController::openSource()
{
    if (m_busy) {
        setLogText("Another operation is already running.");
        return;
    }
    const QString path = QFileDialog::getOpenFileName(
        nullptr,
        "Open Video",
        QString(),
        "Video Files (*.mov *.mp4 *.mkv *.m4v);;All Files (*)");
    if (path.isEmpty()) {
        setLogText("Open video cancelled.");
        return;
    }
    setSourcePath(path);
    probeSource(path);
}

void AppController::chooseOutput()
{
    const QString suggested = m_sourcePath.isEmpty()
        ? QStringLiteral("rewrapped.mov")
        : QFileInfo(m_sourcePath).completeBaseName() + QStringLiteral("-rewrapped.mov");
    const QString path = QFileDialog::getSaveFileName(
        nullptr,
        "Output File",
        suggested,
        "QuickTime Movie (*.mov);;MP4 Video (*.mp4);;Matroska Video (*.mkv);;All Files (*)");
    if (path.isEmpty()) {
        setLogText("Output selection cancelled.");
        return;
    }
    setOutputPath(path);
}

void AppController::recheckTools()
{
    m_ffmpegReady = programExists("ffmpeg");
    m_ffprobeReady = programExists("ffprobe");
    m_ffplayReady = programExists("ffplay");
    m_lastToolCheckMs = QDateTime::currentMSecsSinceEpoch();
    emit toolsChanged();
}

// programExists() runs three blocking QProcess "-version" probes that can stack
// to multiple seconds in pathological PATH/AV/Gatekeeper scenarios. The probes
// run on the GUI thread (these are QML slot entry points), so a freshly-cached
// result is reused for repeat clicks within a short window to avoid stacking.
void AppController::recheckToolsCached()
{
    constexpr qint64 kToolCacheTtlMs = 5'000;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (m_lastToolCheckMs != 0 && now - m_lastToolCheckMs < kToolCacheTtlMs) {
        return;
    }
    recheckTools();
}

void AppController::recheckToolsBackground()
{
    QThread *worker = QThread::create([this] {
        const bool ffmpegOk = programExists("ffmpeg");
        const bool ffprobeOk = programExists("ffprobe");
        const bool ffplayOk = programExists("ffplay");
        const qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
        QMetaObject::invokeMethod(this, [this, ffmpegOk, ffprobeOk, ffplayOk, timestamp] {
            m_ffmpegReady = ffmpegOk;
            m_ffprobeReady = ffprobeOk;
            m_ffplayReady = ffplayOk;
            m_lastToolCheckMs = timestamp;
            emit toolsChanged();
            setLogText(toolStatusText());
        }, Qt::QueuedConnection);
    });
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

void AppController::saveProject()
{
    runExportFlow(
        "Save Rewrap Project",
        "SEDER Rewrap Project (*.json);;All Files (*)",
        "video-rewrap.seder-rewrap.json",
        [this] { return RustBridge::projectJson(m_sourcePath, m_outputPath, m_segments->toJsonArray()); },
        "projectJson",
        "Project save cancelled.",
        "Project saved");
}

bool AppController::runExportFlow(
    const QString &dialogTitle,
    const QString &dialogFilter,
    const QString &defaultFilename,
    const std::function<QJsonObject()> &bridgeCall,
    const QString &payloadKey,
    const QString &cancelLog,
    const QString &successLog)
{
    const QString path = QFileDialog::getSaveFileName(
        nullptr,
        dialogTitle,
        defaultFilename,
        dialogFilter);
    if (path.isEmpty()) {
        setLogText(cancelLog);
        return false;
    }

    const QJsonObject reply = bridgeCall();
    if (!reply.value("ok").toBool()) {
        setLogText(reply.value("error").toString());
        return false;
    }

    if (!writeTextFile(path, reply.value(payloadKey).toString())) {
        return false;
    }

    setLogText(QStringLiteral("%1: %2").arg(successLog, displayPath(path)));
    return true;
}

void AppController::loadProject()
{
    const QString path = QFileDialog::getOpenFileName(
        nullptr,
        "Load Rewrap Project",
        QString(),
        "SEDER Rewrap Project (*.json);;All Files (*)");
    if (path.isEmpty()) {
        setLogText("Project load cancelled.");
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setLogText(QStringLiteral("Unable to read project: %1").arg(file.errorString()));
        return;
    }

    const QJsonObject reply = RustBridge::parseProjectJson(QString::fromUtf8(file.readAll()));
    if (!reply.value("ok").toBool()) {
        setLogText(reply.value("error").toString());
        return;
    }

    const QJsonObject project = reply.value("project").toObject();
    setOutputPath(project.value("output_file").toString());
    m_segments->fromJsonArray(project.value("segments").toArray());
    updateTotalDuration();
    emit segmentsChanged();

    const QString source = project.value("source_file").toString();
    if (!source.isEmpty()) {
        setSourcePath(source);
        probeSource(source);
    }
    setLogText(QStringLiteral("Project loaded: %1").arg(displayPath(path)));
}

void AppController::exportTxtReport()
{
    runExportFlow(
        "Export TXT Report",
        "Text Report (*.txt);;All Files (*)",
        "video-rewrap-report.txt",
        [this] { return RustBridge::rewrapReportTxt(m_sourcePath, m_outputPath, m_segments->toJsonArray()); },
        "report",
        "TXT report export cancelled.",
        "TXT report exported");
}

void AppController::exportCsvReport()
{
    runExportFlow(
        "Export CSV Report",
        "CSV Report (*.csv);;All Files (*)",
        "video-rewrap-report.csv",
        [this] { return RustBridge::rewrapReportCsv(m_sourcePath, m_outputPath, m_segments->toJsonArray()); },
        "report",
        "CSV report export cancelled.",
        "CSV report exported");
}

void AppController::startExport()
{
    if (!ensureCanExport()) {
        return;
    }

    const QJsonObject preflight = RustBridge::rewrapPreflight(m_metadataJson, m_outputPath, m_segments->toJsonArray(), keyframesJson());
    if (!preflight.value("ok").toBool()) {
        setLogText(preflight.value("error").toString());
        return;
    }
    setLogText("Preflight passed. No re-encode fallback is available; stream-copy export will proceed.");

    const QString tempRoot = QDir::temp().filePath(QStringLiteral("seder-video-rewrap-%1-%2")
        .arg(QCoreApplication::applicationPid())
        .arg(QDateTime::currentMSecsSinceEpoch()));
    const QJsonObject plan = RustBridge::exportPlan(
        m_sourcePath,
        m_outputPath,
        tempRoot,
        m_segments->toJsonArray(),
        keyframesJson());
    if (!plan.value("ok").toBool()) {
        setLogText(plan.value("error").toString());
        return;
    }

    m_cancelExport.store(false, std::memory_order_relaxed);
    setBusy(true);
    setProgress(0.05);
    setLogText("Exporting with FFmpeg stream copy only...");

    QThread *worker = QThread::create([this, tempRoot, plan] {
        QDir().mkpath(tempRoot);
        const QJsonArray plannedSegments = plan.value("segments").toArray();
        for (int i = 0; i < plannedSegments.size(); ++i) {
            if (m_cancelExport.load(std::memory_order_relaxed)) {
                QDir(tempRoot).removeRecursively();
                QMetaObject::invokeMethod(this, [this] {
                    setBusy(false);
                    setProgress(0.0);
                    setLogText("Export cancelled.");
                }, Qt::QueuedConnection);
                return;
            }
            const QJsonObject item = plannedSegments.at(i).toObject();
            const QString path = item.value("path").toString();
            const RustBridge::Command command = RustBridge::commandFromJson(item.value("command").toObject());
            QMetaObject::invokeMethod(this, [this, i, plannedSegments] {
                setProgress(static_cast<double>(i) / static_cast<double>(plannedSegments.size() + 1));
                setLogText(QStringLiteral("Exporting segment %1 of %2...")
                    .arg(i + 1)
                    .arg(plannedSegments.size()));
            }, Qt::QueuedConnection);
            const ProcessResult result = runCommand(command, &m_cancelExport);
            if (!result.ok) {
                QDir(tempRoot).removeRecursively();
                QMetaObject::invokeMethod(this, [this, path, result] {
                    setBusy(false);
                    setProgress(0.0);
                    setLogText(QStringLiteral("Segment export failed for %1\n%2")
                        .arg(displayPath(path), result.stderrText));
                }, Qt::QueuedConnection);
                return;
            }
        }

        QFile listFile(plan.value("listPath").toString());
        if (!listFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            const QString error = listFile.errorString();
            QDir(tempRoot).removeRecursively();
            QMetaObject::invokeMethod(this, [this, error] {
                setBusy(false);
                setProgress(0.0);
                setLogText(QStringLiteral("Unable to write concat list: %1").arg(error));
            }, Qt::QueuedConnection);
            return;
        }
        listFile.write(plan.value("listText").toString().toUtf8());
        listFile.close();

        QMetaObject::invokeMethod(this, [this] {
            setProgress(0.9);
            setLogText("Concatenating enabled segments...");
        }, Qt::QueuedConnection);
        const RustBridge::Command concat = RustBridge::commandFromJson(plan.value("concatCommand").toObject());
        const ProcessResult result = runCommand(concat, &m_cancelExport);
        QDir(tempRoot).removeRecursively();

        QMetaObject::invokeMethod(this, [this, result] {
            setBusy(false);
            if (result.ok) {
                setProgress(1.0);
                setLogText(QStringLiteral("Export complete: %1").arg(displayPath(m_outputPath)));
            } else {
                setProgress(0.0);
                setLogText(QStringLiteral("Export failed. No re-encode fallback was attempted.\n%1")
                    .arg(result.stderrText));
            }
        }, Qt::QueuedConnection);
    });
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

void AppController::cancelExport()
{
    if (!m_busy) {
        return;
    }
    m_cancelExport.store(true, std::memory_order_relaxed);
    setLogText("Cancelling export...");
}

void AppController::jumpToTimecode(const QString &timecode)
{
    const QJsonObject parsed = RustBridge::parseTimecode(timecode);
    if (!parsed.value("ok").toBool()) {
        setLogText(parsed.value("error").toString());
        return;
    }
    const qint64 requested = static_cast<qint64>(parsed.value("milliseconds").toDouble());
    const QJsonObject snap = RustBridge::nearestKeyframe(keyframesJson(), requested);
    if (!snap.value("ok").toBool()) {
        setLogText(snap.value("error").toString());
        return;
    }
    const qint64 snapped = static_cast<qint64>(snap.value("snap").toObject().value("snapped_ms").toDouble());
    const auto it = std::lower_bound(m_keyframes.begin(), m_keyframes.end(), snapped);
    if (it != m_keyframes.end() && *it == snapped) {
        m_currentIndex = static_cast<int>(std::distance(m_keyframes.begin(), it));
        emit keyframesChanged();
    }
    const qint64 distance = static_cast<qint64>(snap.value("snap").toObject().value("distance_ms").toDouble());
    setLogText(QStringLiteral("Requested %1 snapped to %2. Distance: %3 ms.")
        .arg(formatMs(requested), formatMs(snapped))
        .arg(distance));
}

void AppController::previousKeyframe()
{
    if (m_currentIndex > 0) {
        --m_currentIndex;
        emit keyframesChanged();
    }
}

void AppController::nextKeyframe()
{
    if (m_currentIndex + 1 < m_keyframes.size()) {
        ++m_currentIndex;
        emit keyframesChanged();
    }
}

void AppController::previewCurrent()
{
    recheckToolsCached();
    if (!m_ffplayReady) {
        setLogText(toolStatusText());
        return;
    }
    if (m_sourcePath.isEmpty() || m_keyframes.isEmpty()) {
        setLogText("Choose and probe a source video first.");
        return;
    }
    const QJsonObject reply = RustBridge::ffplayPreviewCommand(m_sourcePath, currentKeyframeMs());
    if (!reply.value("ok").toBool()) {
        setLogText(reply.value("error").toString());
        return;
    }
    const RustBridge::Command command = RustBridge::commandFromJson(reply.value("command").toObject());
    if (QProcess::startDetached(command.program, command.args)) {
        setLogText("Preview opened in FFplay.");
    } else {
        setLogText("Unable to open FFplay.");
    }
}

void AppController::setIn()
{
    if (m_keyframes.isEmpty()) {
        setLogText("Probe a video before setting an in point.");
        return;
    }
    m_pendingIn = currentKeyframeMs();
    m_hasPendingIn = true;
    emit markersChanged();
}

void AppController::setOut()
{
    if (m_keyframes.isEmpty()) {
        setLogText("Probe a video before setting an out point.");
        return;
    }
    m_pendingOut = currentKeyframeMs();
    m_hasPendingOut = true;
    emit markersChanged();
}

void AppController::addSegment(const QString &name, const QString &notes)
{
    if (!m_hasPendingIn || !m_hasPendingOut) {
        setLogText("Set both in and out keyframes first.");
        return;
    }
    const QString segmentName = name.trimmed().isEmpty()
        ? QStringLiteral("Segment %1").arg(m_segments->rowCount() + 1)
        : name.trimmed();
    SegmentRow segment { segmentName, m_pendingIn, m_pendingOut, notes, true };
    QJsonArray candidate = m_segments->toJsonArray();
    candidate.push_back(QJsonObject {
        { "name", segment.name },
        { "in_ms", static_cast<double>(segment.inMs) },
        { "out_ms", static_cast<double>(segment.outMs) },
        { "notes", segment.notes },
        { "enabled", segment.enabled },
    });
    const QJsonObject validated = RustBridge::validateSegments(candidate, keyframesJson());
    if (!validated.value("ok").toBool()) {
        setLogText(validated.value("error").toString());
        return;
    }
    m_segments->append(segment);
    updateTotalDuration();
    emit segmentsChanged();
    setLogText("Segment added.");
}

void AppController::selectSegment(int row)
{
    setSelectedRowValue(row);
}

void AppController::removeSegment(int row)
{
    m_segments->remove(row);
    if (m_selectedRow == row) {
        setSelectedRowValue(-1);
    } else if (m_selectedRow > row) {
        setSelectedRowValue(m_selectedRow - 1);
    }
    updateTotalDuration();
    emit segmentsChanged();
}

void AppController::duplicateSegment(int row)
{
    m_segments->duplicate(row);
    updateTotalDuration();
    emit segmentsChanged();
}

void AppController::moveSegmentUp(int row)
{
    if (row <= 0) {
        return;
    }
    m_segments->moveUp(row);
    if (m_selectedRow == row) {
        setSelectedRowValue(row - 1);
    }
    updateTotalDuration();
    emit segmentsChanged();
}

void AppController::moveSegmentDown(int row)
{
    if (row < 0 || row + 1 >= m_segments->rowCount()) {
        return;
    }
    m_segments->moveDown(row);
    if (m_selectedRow == row) {
        setSelectedRowValue(row + 1);
    }
    updateTotalDuration();
    emit segmentsChanged();
}

void AppController::toggleSegment(int row, bool enabled)
{
    m_segments->setEnabled(row, enabled);
    updateTotalDuration();
    emit segmentsChanged();
}

void AppController::setTheme(const QString &theme)
{
    const QString normalized = (theme == "light" || theme == "dark") ? theme : QStringLiteral("system");
    const bool dark = normalized == "dark"
        || (normalized == "system" && QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark);
    const bool changed = normalized != m_theme || dark != m_darkMode;
    m_theme = normalized;
    m_darkMode = dark;
    QSettings().setValue("theme", m_theme);
    if (changed) {
        emit themeChanged();
    }
}

void AppController::setSourcePath(const QString &path)
{
    if (m_sourcePath == path) {
        return;
    }
    m_sourcePath = path;
    emit sourcePathChanged();
}

void AppController::setOutputPath(const QString &path)
{
    if (m_outputPath == path) {
        return;
    }
    m_outputPath = path;
    emit outputPathChanged();
}

void AppController::setLogText(const QString &text)
{
    if (m_logText == text) {
        return;
    }
    m_logText = text;
    emit logTextChanged();
}

void AppController::setBusy(bool busy)
{
    if (m_busy == busy) {
        return;
    }
    m_busy = busy;
    emit busyChanged();
}

void AppController::setProgress(double progress)
{
    const double clamped = std::clamp(progress, 0.0, 1.0);
    if (m_progress == clamped) {
        return;
    }
    m_progress = clamped;
    emit progressChanged();
}

void AppController::setSelectedRowValue(int row)
{
    if (m_selectedRow == row) {
        return;
    }
    m_selectedRow = row;
    emit selectedRowChanged();
}

void AppController::probeSource(const QString &path)
{
    clearMediaState();
    if (!m_ffprobeReady) {
        setLogText(toolStatusText());
        return;
    }
    setBusy(true);
    setProgress(0.1);
    setLogText("Probing video metadata and keyframes...");

    QThread *worker = QThread::create([this, path] {
        const QJsonObject metadataReply = RustBridge::ffprobeMetadataCommand(path);
        const QJsonObject keyframeReply = RustBridge::ffprobeKeyframeCommand(path);
        if (!metadataReply.value("ok").toBool() || !keyframeReply.value("ok").toBool()) {
            const QString error = metadataReply.value("error").toString(keyframeReply.value("error").toString());
            QMetaObject::invokeMethod(this, [this, error] {
                setBusy(false);
                setProgress(0.0);
                setLogText(error);
            }, Qt::QueuedConnection);
            return;
        }

        const ProcessResult metadata = runCommand(RustBridge::commandFromJson(metadataReply.value("command").toObject()));
        const ProcessResult keyframes = runCommand(RustBridge::commandFromJson(keyframeReply.value("command").toObject()));
        if (!metadata.ok || !keyframes.ok) {
            const QString error = !metadata.ok ? metadata.stderrText : keyframes.stderrText;
            QMetaObject::invokeMethod(this, [this, error] {
                setBusy(false);
                setProgress(0.0);
                setLogText(QStringLiteral("Probe failed: %1").arg(error));
            }, Qt::QueuedConnection);
            return;
        }

        const quint64 size = QFileInfo(path).size();
        const QJsonObject parsed = RustBridge::parseProbeResult(
            path,
            size,
            metadata.stdoutText,
            keyframes.stdoutText);
        QMetaObject::invokeMethod(this, [this, parsed] {
            setBusy(false);
            if (!parsed.value("ok").toBool()) {
                setProgress(0.0);
                setLogText(QStringLiteral("Probe failed: %1").arg(parsed.value("error").toString()));
                return;
            }
            applyMetadata(parsed.value("metadata").toObject(), parsed.value("keyframes").toArray());
            setProgress(0.0);
            setLogText(QStringLiteral("Detected %1 keyframes.").arg(m_keyframes.size()));
        }, Qt::QueuedConnection);
    });
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

void AppController::clearMediaState()
{
    m_metadataJson = QJsonObject();
    m_keyframes.clear();
    m_currentIndex = 0;
    m_hasPendingIn = false;
    m_hasPendingOut = false;
    m_mediaFilename = "No source";
    m_durationText = "N/A";
    m_resolutionText = "N/A";
    m_codecText = "N/A";
    m_sizeText = "N/A";
    m_mediaSummary = "Open a video to inspect stream-copy-safe keyframes.";
    emit metadataChanged();
    emit keyframesChanged();
    emit markersChanged();
}

void AppController::applyMetadata(const QJsonObject &metadata, const QJsonArray &keyframes)
{
    m_metadataJson = metadata;
    m_mediaFilename = metadata.value("filename").toString("Unknown");
    const QJsonValue duration = metadata.value("duration_ms");
    m_durationText = duration.isNull() ? QStringLiteral("N/A") : formatMs(static_cast<qint64>(duration.toDouble()));
    const int width = metadata.value("width").toInt();
    const int height = metadata.value("height").toInt();
    m_resolutionText = width > 0 && height > 0 ? QStringLiteral("%1x%2").arg(width).arg(height) : QStringLiteral("N/A");
    m_codecText = metadata.value("codec").toString("N/A");
    m_sizeText = bytesText(static_cast<quint64>(metadata.value("file_size").toDouble()));
    m_mediaSummary = QStringLiteral("%1 / %2 / %3")
        .arg(m_mediaFilename,
             metadata.value("container").toString("unknown container"),
             metadata.value("frame_rate").toString("unknown fps"));

    m_keyframes.clear();
    m_keyframes.reserve(keyframes.size());
    for (const QJsonValue &value : keyframes) {
        m_keyframes.push_back(static_cast<qint64>(value.toDouble()));
    }
    m_currentIndex = 0;
    emit metadataChanged();
    emit keyframesChanged();
}

void AppController::updateTotalDuration()
{
    qint64 total = 0;
    for (const SegmentRow &segment : m_segments->segments()) {
        if (segment.enabled) {
            total += std::max<qint64>(0, segment.outMs - segment.inMs);
        }
    }
    m_totalDurationText = formatMs(total);
}

QString AppController::formatMs(qint64 milliseconds)
{
    const qint64 safe = std::max<qint64>(0, milliseconds);
    const qint64 hours = safe / 3600000;
    const qint64 minutes = (safe % 3600000) / 60000;
    const qint64 seconds = (safe % 60000) / 1000;
    const qint64 millis = safe % 1000;
    return QStringLiteral("%1:%2:%3.%4")
        .arg(hours, 2, 10, QLatin1Char('0'))
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'))
        .arg(millis, 3, 10, QLatin1Char('0'));
}

QString AppController::displayPath(const QString &path) const
{
    return QDir::toNativeSeparators(path);
}

QString AppController::toolStatusText() const
{
    QStringList missing;
    if (!m_ffmpegReady) missing.push_back("ffmpeg");
    if (!m_ffprobeReady) missing.push_back("ffprobe");
    if (!m_ffplayReady) missing.push_back("ffplay");
    if (missing.isEmpty()) {
        return "FFmpeg, FFprobe, and FFplay detected.";
    }
    return QStringLiteral("Missing %1. Install FFmpeg from ffmpeg.org or with Homebrew: brew install ffmpeg.")
        .arg(missing.join(", "));
}

QJsonArray AppController::keyframesJson() const
{
    QJsonArray array;
    for (qint64 keyframe : m_keyframes) {
        array.push_back(static_cast<double>(keyframe));
    }
    return array;
}

qint64 AppController::currentKeyframeMs() const
{
    if (m_keyframes.isEmpty() || m_currentIndex < 0 || m_currentIndex >= m_keyframes.size()) {
        return 0;
    }
    return m_keyframes.at(m_currentIndex);
}

bool AppController::programExists(const QString &program)
{
    QProcess process;
    process.start(program, { "-version" });
    if (!process.waitForStarted(1000)) {
        return false;
    }
    if (!process.waitForFinished(1500)) {
        process.kill();
        process.waitForFinished();
        return false;
    }
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

AppController::ProcessResult AppController::runCommand(const RustBridge::Command &command, std::atomic_bool *cancel)
{
    ProcessResult result;
    if (command.program.isEmpty()) {
        result.stderrText = "Empty process command.";
        return result;
    }

    QProcess process;
    process.start(command.program, command.args);
    if (!process.waitForStarted(10'000)) {
        result.stderrText = QStringLiteral("Unable to start %1").arg(command.program);
        return result;
    }

    QByteArray stdoutBuf;
    QByteArray stderrBuf;
    while (!process.waitForFinished(100)) {
        stdoutBuf += process.readAllStandardOutput();
        stderrBuf += process.readAllStandardError();
        if (cancel && cancel->load(std::memory_order_relaxed)) {
            process.kill();
            process.waitForFinished();
            result.stderrText = "Process cancelled.";
            return result;
        }
    }
    stdoutBuf += process.readAllStandardOutput();
    stderrBuf += process.readAllStandardError();

    result.stdoutText = QString::fromUtf8(stdoutBuf);
    result.stderrText = QString::fromUtf8(stderrBuf).trimmed();
    result.exitCode = process.exitCode();
    result.ok = process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
    if (!result.ok && result.stderrText.isEmpty()) {
        result.stderrText = QStringLiteral("%1 exited with code %2").arg(command.program).arg(process.exitCode());
    }
    return result;
}

QString AppController::bytesText(quint64 bytes)
{
    const char *units[] = { "B", "KB", "MB", "GB", "TB" };
    double value = static_cast<double>(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        ++unit;
    }
    if (unit == 0) {
        return QStringLiteral("%1 B").arg(bytes);
    }
    return QStringLiteral("%1 %2").arg(value, 0, 'f', 1).arg(units[unit]);
}

bool AppController::ensureCanExport()
{
    if (m_busy) {
        setLogText("Another operation is already running.");
        return false;
    }
    recheckToolsCached();
    if (!m_ffmpegReady || !m_ffprobeReady) {
        setLogText(toolStatusText());
        return false;
    }
    if (m_sourcePath.isEmpty()) {
        setLogText("Choose a source video first.");
        return false;
    }
    if (m_outputPath.isEmpty()) {
        setLogText("Choose an output file first.");
        return false;
    }
    if (m_segments->rowCount() == 0) {
        setLogText("Add at least one segment before exporting.");
        return false;
    }
    const QJsonObject validated = RustBridge::validateSegments(m_segments->toJsonArray(), keyframesJson());
    if (!validated.value("ok").toBool()) {
        setLogText(validated.value("error").toString());
        return false;
    }
    return true;
}

bool AppController::writeTextFile(const QString &path, const QString &contents)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        setLogText(QStringLiteral("Unable to write %1: %2").arg(displayPath(path), file.errorString()));
        return false;
    }
    file.write(contents.toUtf8());
    file.flush();
    if (file.error() != QFileDevice::NoError) {
        setLogText(QStringLiteral("Unable to flush %1: %2").arg(displayPath(path), file.errorString()));
        return false;
    }
    return true;
}
