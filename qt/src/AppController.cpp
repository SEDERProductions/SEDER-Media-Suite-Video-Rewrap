#include "AppController.h"

#include "RustBridge.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QProcess>
#include <QSettings>
#include <QStyleHints>
#include <algorithm>

namespace {
QString formatMs(qint64 milliseconds)
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

QString bytesText(quint64 bytes)
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

void killPreviewPids(QVector<qint64> &pids)
{
    for (const qint64 pid : pids) {
#if defined(Q_OS_WIN)
        QProcess::execute("taskkill", { "/PID", QString::number(pid), "/F" });
#else
        QProcess::execute("kill", { QString::number(pid) });
#endif
    }
    pids.clear();
}
}

AppController::AppController(SegmentTableModel *segments, QObject *parent)
    : QObject(parent)
    , m_segments(segments)
{
    m_probeEngine = new ProbeEngine(this);
    m_exportEngine = new ExportEngine(this);

    // Wire ProbeEngine signals
    connect(m_probeEngine, &ProbeEngine::toolsChanged, this, [this] {
        emit toolsChanged();
    });
    connect(m_probeEngine, &ProbeEngine::logMessage, this, &AppController::setLogText);
    connect(m_probeEngine, &ProbeEngine::probeComplete, this, [this](bool ok, QJsonObject metadata, QJsonArray keyframes, QString error) {
        if (ok) {
            applyMetadata(metadata, keyframes);
            setLogText(QStringLiteral("Detected %1 keyframes.").arg(m_keyframes.size()));
        } else {
            setLogText(error);
        }
    });

    // Wire ExportEngine signals
    connect(m_exportEngine, &ExportEngine::busyChanged, this, [this] {
        emit busyChanged();
    });
    connect(m_exportEngine, &ExportEngine::progressChanged, this, [this] {
        emit progressChanged();
    });
    connect(m_exportEngine, &ExportEngine::exportModeChanged, this, [this] {
        emit exportModeChanged();
    });
    connect(m_exportEngine, &ExportEngine::logMessage, this, &AppController::setLogText);

    // Restore theme and start background tool check
    QSettings settings;
    m_theme = settings.value("theme", "system").toString();
    setTheme(m_theme);
    if (QGuiApplication::instance()) {
        connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, [this] {
            if (m_theme == "system") {
                setTheme("system");
            }
        });
    }
    m_probeEngine->recheckBackground();

    // Clean up preview processes on exit
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, [this] {
        killPreviewPids(m_previewPids);
    });
}

SegmentTableModel *AppController::segmentModel() const { return m_segments; }
QString AppController::sourcePath() const { return m_sourcePath; }
QString AppController::outputPath() const { return m_outputPath; }
QString AppController::logText() const { return m_logText; }
bool AppController::busy() const { return m_exportEngine->busy(); }
double AppController::progress() const { return m_exportEngine->progress(); }
bool AppController::ffmpegReady() const { return m_probeEngine->ffmpegReady(); }
bool AppController::ffprobeReady() const { return m_probeEngine->ffprobeReady(); }
bool AppController::ffplayReady() const { return m_probeEngine->ffplayReady(); }
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
QString AppController::exportMode() const { return m_exportEngine->exportMode(); }

void AppController::openSource()
{
    if (m_exportEngine->busy()) {
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
    clearMediaState();
    m_probeEngine->probeSource(path);
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
    m_probeEngine->recheckTools();
    setLogText(QStringLiteral("FFmpeg: %1, FFprobe: %2, FFplay: %3")
        .arg(m_probeEngine->ffmpegReady() ? "yes" : "no")
        .arg(m_probeEngine->ffprobeReady() ? "yes" : "no")
        .arg(m_probeEngine->ffplayReady() ? "yes" : "no"));
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

    setLogText(QStringLiteral("%1: %2").arg(successLog, QDir::toNativeSeparators(path)));
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
        clearMediaState();
        m_probeEngine->probeSource(source);
    }
    setLogText(QStringLiteral("Project loaded: %1").arg(QDir::toNativeSeparators(path)));
}

void AppController::exportTxtReport()
{
    runExportFlow(
        "Export TXT Report",
        "Text Report (*.txt);;All Files (*)",
        "video-rewrap-report.txt",
        [this] { return RustBridge::rewrapReportTxt(m_sourcePath, m_outputPath, m_segments->toJsonArray(), m_exportEngine->exportMode()); },
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
        [this] { return RustBridge::rewrapReportCsv(m_sourcePath, m_outputPath, m_segments->toJsonArray(), m_exportEngine->exportMode()); },
        "report",
        "CSV report export cancelled.",
        "CSV report exported");
}

void AppController::startExport()
{
    m_exportEngine->resetOverwriteSession();
    if (!m_exportEngine->ensureCanExport(
            m_probeEngine->ffmpegReady(),
            m_probeEngine->ffprobeReady(),
            m_sourcePath,
            m_outputPath,
            m_segments,
            keyframesJson(),
            m_exportEngine->busy())) {
        return;
    }
    m_exportEngine->startExport(
        m_sourcePath,
        m_outputPath,
        m_metadataJson,
        m_segments,
        keyframesJson(),
        m_exportEngine->overwriteApprovedForSession());
}

void AppController::cancelExport()
{
    m_exportEngine->cancel();
}

void AppController::replaceFile()
{
    if (m_exportEngine->busy()) {
        setLogText("Another operation is already running.");
        return;
    }
    m_probeEngine->recheckCached();
    if (!ffmpegReady() || !ffprobeReady()) {
        setLogText("FFmpeg and/or FFprobe are missing.");
        return;
    }
    if (m_sourcePath.isEmpty()) {
        setLogText("Choose a source video first.");
        return;
    }

    const QFileInfo sourceInfo(m_sourcePath);
    if (!sourceInfo.exists()) {
        setLogText("Source file no longer exists.");
        return;
    }

    const QString dir = sourceInfo.absolutePath();
    const QString baseName = sourceInfo.completeBaseName();
    const QString ext = sourceInfo.suffix();
    const QString backupPath = ext.isEmpty()
        ? QStringLiteral("%1/%2_PREWRAP").arg(dir, baseName)
        : QStringLiteral("%1/%2_PREWRAP.%3").arg(dir, baseName, ext);

    if (QFileInfo::exists(backupPath)) {
        setLogText(QStringLiteral("Cannot replace: _PREWRAP backup already exists at %1. Remove it first or rename the source.").arg(QDir::toNativeSeparators(backupPath)));
        return;
    }

    QFile file(m_sourcePath);
    if (!file.rename(backupPath)) {
        setLogText(QStringLiteral("Unable to rename source to backup: %1").arg(file.errorString()));
        return;
    }

    const QString originalSource = m_sourcePath;
    m_sourcePath = backupPath;
    m_outputPath = originalSource;
    emit sourcePathChanged();
    emit outputPathChanged();
    setLogText(QStringLiteral("Original backed up to %1. Output set to original path %2. Exporting...")
        .arg(QDir::toNativeSeparators(backupPath), QDir::toNativeSeparators(originalSource)));

    m_exportEngine->resetOverwriteSession();
    m_exportEngine->startExport(
        backupPath,
        originalSource,
        m_metadataJson,
        m_segments,
        keyframesJson(),
        true);
}

void AppController::setExportMode(const QString &mode)
{
    m_exportEngine->setExportMode(mode);
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
    m_probeEngine->recheckCached();
    if (!ffplayReady()) {
        setLogText("FFplay is not available. Install FFmpeg to enable preview.");
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

    // Kill previous preview before launching a new one
    killPreviewPids(m_previewPids);

    qint64 pid = 0;
    if (QProcess::startDetached(command.program, command.args, QString(), &pid)) {
        if (pid > 0) {
            m_previewPids.push_back(pid);
        }
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
    if (row <= 0) return;
    m_segments->moveUp(row);
    if (m_selectedRow == row) {
        setSelectedRowValue(row - 1);
    }
    updateTotalDuration();
    emit segmentsChanged();
}

void AppController::moveSegmentDown(int row)
{
    if (row < 0 || row + 1 >= m_segments->rowCount()) return;
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
    if (m_sourcePath == path) return;
    m_sourcePath = path;
    emit sourcePathChanged();
}

void AppController::setOutputPath(const QString &path)
{
    if (m_outputPath == path) return;
    m_outputPath = path;
    emit outputPathChanged();
}

void AppController::setLogText(const QString &text)
{
    if (m_logText == text) return;
    m_logText = text;
    emit logTextChanged();
}

void AppController::setSelectedRowValue(int row)
{
    if (m_selectedRow == row) return;
    m_selectedRow = row;
    emit selectedRowChanged();
}

void AppController::clearMediaState()
{
    m_probeEngine->clearState();
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

bool AppController::writeTextFile(const QString &path, const QString &contents)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        setLogText(QStringLiteral("Unable to write %1: %2").arg(QDir::toNativeSeparators(path), file.errorString()));
        return false;
    }
    file.write(contents.toUtf8());
    file.flush();
    if (file.error() != QFileDevice::NoError) {
        setLogText(QStringLiteral("Unable to flush %1: %2").arg(QDir::toNativeSeparators(path), file.errorString()));
        return false;
    }
    return true;
}

bool AppController::ensureCanExportForTesting()
{
    return m_exportEngine->ensureCanExport(
        m_probeEngine->ffmpegReady(),
        m_probeEngine->ffprobeReady(),
        m_sourcePath,
        m_outputPath,
        m_segments,
        keyframesJson(),
        m_exportEngine->busy());
}

void AppController::setPathsForTesting(const QString &source, const QString &output)
{
    m_sourcePath = source;
    m_outputPath = output;
}

void AppController::setToolsReadyForTesting(bool ffmpegReady, bool ffprobeReady)
{
    // Re-run tool check so probe engine picks up the state we want
    // (these are overridden by ProcessUtil::programExists normally)
    Q_UNUSED(ffmpegReady);
    Q_UNUSED(ffprobeReady);
}

void AppController::setOverwriteDecisionProviderForTesting(const std::function<bool(const QString &)> &provider)
{
    m_exportEngine->setOverwriteDecisionProviderForTesting(provider);
}

bool AppController::overwriteApprovedForSessionForTesting() const
{
    return m_exportEngine->overwriteApprovedForSession();
}

void AppController::setKeyframesForTesting(const QVector<qint64> &keyframes)
{
    m_keyframes = keyframes;
}
