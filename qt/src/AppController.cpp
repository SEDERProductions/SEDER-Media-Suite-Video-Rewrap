#include "AppController.h"

#include "ProcessUtil.h"
#include "RustBridge.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QPointer>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QStyleHints>
#include <QThread>
#include <QUrl>
#include <algorithm>

namespace {
constexpr auto kRecentSourcesKey = "recent/sources";
constexpr auto kRecentOutputsKey = "recent/outputs";
constexpr auto kRecentProjectsKey = "recent/projects";
constexpr auto kCustomFfmpegDirKey = "ffmpeg/customDir";

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

bool looksLikeProjectFile(const QString &path)
{
    const QFileInfo info(path);
    if (info.suffix().compare("json", Qt::CaseInsensitive) == 0) {
        const QString name = info.fileName();
        return name.contains(".seder-rewrap", Qt::CaseInsensitive)
            || name.contains("project", Qt::CaseInsensitive);
    }
    return false;
}

QString urlOrPathToLocal(const QString &value)
{
    if (value.isEmpty()) return value;
    if (value.startsWith("file:")) {
        return QUrl(value).toLocalFile();
    }
    return value;
}
}

AppController::AppController(SegmentTableModel *segments, QObject *parent)
    : QObject(parent)
    , m_segments(segments)
    , m_recentSources(new RecentFilesModel(kRecentSourcesKey, this))
    , m_recentOutputs(new RecentFilesModel(kRecentOutputsKey, this))
    , m_recentProjects(new RecentFilesModel(kRecentProjectsKey, this))
    , m_updateChecker(new UpdateChecker(this))
{
    m_probeEngine = new ProbeEngine(this);
    m_exportEngine = new ExportEngine(this);

    connect(m_probeEngine, &ProbeEngine::toolsChanged, this, [this] {
        refreshFfmpegVersion();
        emit toolsChanged();
    });
    connect(m_probeEngine, &ProbeEngine::logMessage, this, &AppController::setLogText);
    connect(m_probeEngine, &ProbeEngine::probeComplete, this, [this](bool ok, QJsonObject metadata, QJsonArray keyframes, QString error) {
        setProbing(false);
        if (ok) {
            applyMetadata(metadata, keyframes);
            setNotice(QString(), QString());
            setLogText(tr("Detected %1 keyframes.").arg(m_keyframes.size()));
        } else {
            setLastErrorLog(error);
            setNotice(error, QStringLiteral("bad"));
            setLogText(error);
        }
    });

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
    connect(m_exportEngine, &ExportEngine::errorReport, this, &AppController::setLastErrorLog);

    connect(&m_undo, &QUndoStack::canUndoChanged, this, [this] { emit undoStackChanged(); });
    connect(&m_undo, &QUndoStack::canRedoChanged, this, [this] { emit undoStackChanged(); });

    QSettings settings;
    m_theme = settings.value("theme", "system").toString();
    m_customFfmpegDir = settings.value(kCustomFfmpegDirKey).toString();
    applyCustomFfmpegDirToEnvironment();
    setTheme(m_theme);
    m_probeEngine->recheckBackground();

    if (m_updateChecker->checkOnLaunch() && qEnvironmentVariableIsEmpty("SEDER_DISABLE_UPDATE_CHECK")) {
        m_updateChecker->checkNow();
    }

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
QString AppController::appVersion() const { return QCoreApplication::applicationVersion(); }

QUrl AppController::videoUrl() const
{
    return m_sourcePath.isEmpty() ? QUrl() : QUrl::fromLocalFile(m_sourcePath);
}

qint64 AppController::durationMs() const
{
    const QJsonValue duration = m_metadataJson.value("duration_ms");
    return duration.isNull() ? 0 : static_cast<qint64>(duration.toDouble());
}

QVariantList AppController::keyframesMs() const
{
    // Cap the markers handed to the QML timeline so dense keyframe sets stay
    // cheap to render; subsample evenly when above the cap.
    constexpr int kMaxMarkers = 500;
    const int count = m_keyframes.size();
    QVariantList list;
    if (count <= kMaxMarkers) {
        list.reserve(count);
        for (const qint64 keyframe : m_keyframes) {
            list.push_back(QVariant::fromValue(keyframe));
        }
    } else {
        list.reserve(kMaxMarkers);
        for (int i = 0; i < kMaxMarkers; ++i) {
            const int index = static_cast<int>(static_cast<qint64>(i) * (count - 1) / (kMaxMarkers - 1));
            list.push_back(QVariant::fromValue(m_keyframes.at(index)));
        }
    }
    return list;
}

qint64 AppController::inMs() const { return m_hasPendingIn ? m_pendingIn : -1; }
qint64 AppController::outMs() const { return m_hasPendingOut ? m_pendingOut : -1; }

void AppController::openSource()
{
    if (m_exportEngine->busy()) {
        setLogText(tr("Another operation is already running."));
        return;
    }
    const QString path = QFileDialog::getOpenFileName(
        nullptr,
        tr("Open Video"),
        QString(),
        tr("Video Files (*.mov *.mp4 *.mkv *.m4v);;All Files (*)"));
    if (path.isEmpty()) {
        setLogText(tr("Open video cancelled."));
        return;
    }
    openSourcePath(path);
}

void AppController::openSourcePath(const QString &path)
{
    if (path.isEmpty()) return;
    setSourcePath(path);
    m_recentSources->prepend(path);
    clearMediaState();
    if (m_probeEngine->ffprobeReady()) {
        setProbing(true);
    }
    m_probeEngine->probeSource(path);
}

void AppController::openProjectPath(const QString &path)
{
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setLogText(tr("Unable to read project: %1").arg(file.errorString()));
        return;
    }
    const QJsonObject reply = RustBridge::parseProjectJson(QString::fromUtf8(file.readAll()));
    if (!reply.value("ok").toBool()) {
        const QString error = reply.value("error").toString();
        setLastErrorLog(error);
        setLogText(error);
        return;
    }

    const QJsonObject project = reply.value("project").toObject();
    m_undo.clear();
    setOutputPath(project.value("output_file").toString());
    m_segments->fromJsonArray(project.value("segments").toArray());
    updateTotalDuration();
    emit segmentsChanged();

    const QString source = project.value("source_file").toString();
    if (!source.isEmpty()) {
        openSourcePath(source);
    }
    m_recentProjects->prepend(path);
    setLogText(tr("Project loaded: %1").arg(QDir::toNativeSeparators(path)));
}

void AppController::setOutputFromPath(const QString &path)
{
    if (path.isEmpty()) return;
    setOutputPath(path);
    m_recentOutputs->prepend(path);
}

void AppController::chooseOutput()
{
    const QString suggested = m_sourcePath.isEmpty()
        ? QStringLiteral("rewrapped.mov")
        : QFileInfo(m_sourcePath).completeBaseName() + QStringLiteral("-rewrapped.mov");
    const QString path = QFileDialog::getSaveFileName(
        nullptr,
        tr("Output File"),
        suggested,
        tr("QuickTime Movie (*.mov);;MP4 Video (*.mp4);;Matroska Video (*.mkv);;All Files (*)"));
    if (path.isEmpty()) {
        setLogText(tr("Output selection cancelled."));
        return;
    }
    setOutputFromPath(path);
}

void AppController::recheckTools()
{
    m_probeEngine->recheckTools();
    refreshFfmpegVersion();
    setLogText(tr("FFmpeg: %1, FFprobe: %2, FFplay: %3")
        .arg(m_probeEngine->ffmpegReady() ? tr("yes") : tr("no"))
        .arg(m_probeEngine->ffprobeReady() ? tr("yes") : tr("no"))
        .arg(m_probeEngine->ffplayReady() ? tr("yes") : tr("no")));
}

void AppController::saveProject()
{
    runExportFlow(
        tr("Save Rewrap Project"),
        tr("SEDER Rewrap Project (*.json);;All Files (*)"),
        "video-rewrap.seder-rewrap.json",
        [this] { return RustBridge::projectJson(m_sourcePath, m_outputPath, m_segments->toJsonArray()); },
        "projectJson",
        tr("Project save cancelled."),
        tr("Project saved"));
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
        const QString error = reply.value("error").toString();
        setLastErrorLog(error);
        setLogText(error);
        return false;
    }

    if (!writeTextFile(path, reply.value(payloadKey).toString())) {
        return false;
    }

    if (payloadKey == QLatin1String("projectJson")) {
        m_recentProjects->prepend(path);
    }
    setLogText(QStringLiteral("%1: %2").arg(successLog, QDir::toNativeSeparators(path)));
    return true;
}

void AppController::loadProject()
{
    const QString path = QFileDialog::getOpenFileName(
        nullptr,
        tr("Load Rewrap Project"),
        QString(),
        tr("SEDER Rewrap Project (*.json);;All Files (*)"));
    if (path.isEmpty()) {
        setLogText(tr("Project load cancelled."));
        return;
    }
    openProjectPath(path);
}

void AppController::exportTxtReport()
{
    runExportFlow(
        tr("Export TXT Report"),
        tr("Text Report (*.txt);;All Files (*)"),
        "video-rewrap-report.txt",
        [this] { return RustBridge::rewrapReportTxt(m_sourcePath, m_outputPath, m_segments->toJsonArray(), m_exportEngine->exportMode()); },
        "report",
        tr("TXT report export cancelled."),
        tr("TXT report exported"));
}

void AppController::exportCsvReport()
{
    runExportFlow(
        tr("Export CSV Report"),
        tr("CSV Report (*.csv);;All Files (*)"),
        "video-rewrap-report.csv",
        [this] { return RustBridge::rewrapReportCsv(m_sourcePath, m_outputPath, m_segments->toJsonArray(), m_exportEngine->exportMode()); },
        "report",
        tr("CSV report export cancelled."),
        tr("CSV report exported"));
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
    if (!m_ffmpegCompatible) {
        setLogText(m_ffmpegVersionText.isEmpty()
            ? tr("FFmpeg version check failed. Resolve before exporting.")
            : m_ffmpegVersionText);
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
        setLogText(tr("Another operation is already running."));
        return;
    }
    m_probeEngine->recheckCached();
    if (!ffmpegReady() || !ffprobeReady()) {
        setLogText(tr("FFmpeg and/or FFprobe are missing."));
        return;
    }
    if (m_sourcePath.isEmpty()) {
        setLogText(tr("Choose a source video first."));
        return;
    }

    const QFileInfo sourceInfo(m_sourcePath);
    if (!sourceInfo.exists()) {
        setLogText(tr("Source file no longer exists."));
        return;
    }

    const QString dir = sourceInfo.absolutePath();
    const QString baseName = sourceInfo.completeBaseName();
    const QString ext = sourceInfo.suffix();
    const QString backupPath = ext.isEmpty()
        ? QStringLiteral("%1/%2_PREWRAP").arg(dir, baseName)
        : QStringLiteral("%1/%2_PREWRAP.%3").arg(dir, baseName, ext);

    if (QFileInfo::exists(backupPath)) {
        setLogText(tr("Cannot replace: _PREWRAP backup already exists at %1. Remove it first or rename the source.")
            .arg(QDir::toNativeSeparators(backupPath)));
        return;
    }

    QFile file(m_sourcePath);
    if (!file.rename(backupPath)) {
        setLogText(tr("Unable to rename source to backup: %1").arg(file.errorString()));
        return;
    }

    const QString originalSource = m_sourcePath;
    m_sourcePath = backupPath;
    m_outputPath = originalSource;
    emit sourcePathChanged();
    emit outputPathChanged();
    setLogText(tr("Original backed up to %1. Output set to original path %2. Exporting...")
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
    setLogText(tr("Requested %1 snapped to %2. Distance: %3 ms.")
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
        setLogText(tr("FFplay is not available. Install FFmpeg to enable preview."));
        return;
    }
    if (m_sourcePath.isEmpty() || m_keyframes.isEmpty()) {
        setLogText(tr("Choose and probe a source video first."));
        return;
    }
    const QJsonObject reply = RustBridge::ffplayPreviewCommand(m_sourcePath, currentKeyframeMs());
    if (!reply.value("ok").toBool()) {
        setLogText(reply.value("error").toString());
        return;
    }
    const RustBridge::Command command = RustBridge::commandFromJson(reply.value("command").toObject());

    killPreviewPids(m_previewPids);

    qint64 pid = 0;
    if (QProcess::startDetached(command.program, command.args, QString(), &pid)) {
        if (pid > 0) {
            m_previewPids.push_back(pid);
        }
        setLogText(tr("Preview opened in FFplay."));
    } else {
        setLogText(tr("Unable to open FFplay."));
    }
}

void AppController::setIn()
{
    if (m_keyframes.isEmpty()) {
        setLogText(tr("Probe a video before setting an in point."));
        return;
    }
    m_pendingIn = currentKeyframeMs();
    m_hasPendingIn = true;
    emit markersChanged();
    generateThumbnail(m_pendingIn, true);
}

void AppController::setOut()
{
    if (m_keyframes.isEmpty()) {
        setLogText(tr("Probe a video before setting an out point."));
        return;
    }
    m_pendingOut = currentKeyframeMs();
    m_hasPendingOut = true;
    emit markersChanged();
    generateThumbnail(m_pendingOut, false);
}

void AppController::addSegment(const QString &name, const QString &notes)
{
    if (!m_hasPendingIn || !m_hasPendingOut) {
        setLogText(tr("Set both in and out keyframes first."));
        return;
    }
    const QString segmentName = name.trimmed().isEmpty()
        ? tr("Segment %1").arg(m_segments->rowCount() + 1)
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
    m_undo.push(new AddSegmentCommand(m_segments, this, segment));
    setLogText(tr("Segment added."));
}

void AppController::selectSegment(int row)
{
    setSelectedRowValue(row);
}

void AppController::removeSegment(int row)
{
    if (row < 0 || row >= m_segments->rowCount()) return;
    m_undo.push(new RemoveSegmentCommand(m_segments, this, row));
    if (m_selectedRow == row) {
        setSelectedRowValue(-1);
    } else if (m_selectedRow > row) {
        setSelectedRowValue(m_selectedRow - 1);
    }
}

void AppController::duplicateSegment(int row)
{
    if (row < 0 || row >= m_segments->rowCount()) return;
    m_undo.push(new DuplicateSegmentCommand(m_segments, this, row));
}

void AppController::moveSegmentUp(int row)
{
    if (row <= 0) return;
    m_undo.push(new MoveSegmentCommand(m_segments, this, row, row - 1));
    if (m_selectedRow == row) {
        setSelectedRowValue(row - 1);
    }
}

void AppController::moveSegmentDown(int row)
{
    if (row < 0 || row + 1 >= m_segments->rowCount()) return;
    m_undo.push(new MoveSegmentCommand(m_segments, this, row, row + 1));
    if (m_selectedRow == row) {
        setSelectedRowValue(row + 1);
    }
}

void AppController::toggleSegment(int row, bool enabled)
{
    if (row < 0 || row >= m_segments->rowCount()) return;
    m_undo.push(new ToggleSegmentCommand(m_segments, this, row, enabled));
}

void AppController::setTheme(const QString &theme)
{
    const QString normalized = (theme == "light" || theme == "dark") ? theme : QStringLiteral("system");
    const bool dark = normalized == "dark";
    const bool changed = normalized != m_theme || dark != m_darkMode;
    m_theme = normalized;
    m_darkMode = dark;
    QSettings().setValue("theme", m_theme);
    if (changed) {
        emit themeChanged();
    }
}

void AppController::undo()
{
    if (m_undo.canUndo()) m_undo.undo();
}

void AppController::redo()
{
    if (m_undo.canRedo()) m_undo.redo();
}

void AppController::handleDroppedFile(const QString &url)
{
    const QString path = urlOrPathToLocal(url);
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        setLogText(tr("Dropped file does not exist: %1").arg(path));
        return;
    }
    if (looksLikeProjectFile(path)) {
        openProjectPath(path);
    } else {
        openSourcePath(path);
    }
}

void AppController::setCustomFfmpegDir(const QString &dir)
{
    const QString normalized = urlOrPathToLocal(dir);
    if (m_customFfmpegDir == normalized) return;
    m_customFfmpegDir = normalized;
    QSettings().setValue(kCustomFfmpegDirKey, m_customFfmpegDir);
    applyCustomFfmpegDirToEnvironment();
    m_probeEngine->recheckTools();
    refreshFfmpegVersion();
    emit customFfmpegDirChanged();
    emit toolsChanged();
}

void AppController::clearCustomFfmpegDir()
{
    setCustomFfmpegDir(QString());
}

void AppController::copyLastErrorLogToClipboard()
{
    if (m_lastErrorLog.isEmpty()) return;
    if (QGuiApplication::clipboard()) {
        QGuiApplication::clipboard()->setText(m_lastErrorLog);
        setLogText(tr("Error log copied to clipboard."));
    }
}

void AppController::onSegmentsMutated()
{
    updateTotalDuration();
    emit segmentsChanged();
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

void AppController::setLastErrorLog(const QString &text)
{
    if (m_lastErrorLog == text) return;
    m_lastErrorLog = text;
    emit lastErrorLogChanged();
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
    m_mediaFilename = tr("No source");
    m_durationText = tr("N/A");
    m_resolutionText = tr("N/A");
    m_codecText = tr("N/A");
    m_sizeText = tr("N/A");
    m_mediaSummary = tr("Open a video to inspect stream-copy-safe keyframes.");
    m_inThumbUrl = QUrl();
    m_outThumbUrl = QUrl();
    m_inThumbFile.clear();
    m_outThumbFile.clear();
    emit metadataChanged();
    emit keyframesChanged();
    emit markersChanged();
    emit thumbnailsChanged();
}

void AppController::applyMetadata(const QJsonObject &metadata, const QJsonArray &keyframes)
{
    m_metadataJson = metadata;
    m_mediaFilename = metadata.value("filename").toString(tr("Unknown"));
    const QJsonValue duration = metadata.value("duration_ms");
    m_durationText = duration.isNull() ? tr("N/A") : formatMs(static_cast<qint64>(duration.toDouble()));
    const int width = metadata.value("width").toInt();
    const int height = metadata.value("height").toInt();
    m_resolutionText = width > 0 && height > 0 ? QStringLiteral("%1x%2").arg(width).arg(height) : tr("N/A");
    m_codecText = metadata.value("codec").toString(tr("N/A"));
    m_sizeText = bytesText(static_cast<quint64>(metadata.value("file_size").toDouble()));
    m_mediaSummary = QStringLiteral("%1 / %2 / %3")
        .arg(m_mediaFilename,
             metadata.value("container").toString(tr("unknown container")),
             metadata.value("frame_rate").toString(tr("unknown fps")));

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
        setLogText(tr("Unable to write %1: %2").arg(QDir::toNativeSeparators(path), file.errorString()));
        return false;
    }
    file.write(contents.toUtf8());
    file.flush();
    if (file.error() != QFileDevice::NoError) {
        setLogText(tr("Unable to flush %1: %2").arg(QDir::toNativeSeparators(path), file.errorString()));
        return false;
    }
    return true;
}

void AppController::refreshFfmpegVersion()
{
    if (!m_probeEngine->ffmpegReady()) {
        m_ffmpegVersionText = tr("FFmpeg not detected.");
        m_ffmpegCompatible = false;
        return;
    }
    const QString output = ProcessUtil::programVersionOutput("ffmpeg");
    if (output.isEmpty()) {
        m_ffmpegVersionText = tr("Unable to read FFmpeg version output.");
        m_ffmpegCompatible = false;
        return;
    }
    const QJsonObject reply = RustBridge::ffmpegCompatibility(output);
    if (!reply.value("ok").toBool()) {
        m_ffmpegVersionText = reply.value("error").toString();
        m_ffmpegCompatible = false;
        return;
    }
    const QJsonObject compat = reply.value("compatibility").toObject();
    m_ffmpegCompatible = compat.value("compatible").toBool();
    m_ffmpegVersionText = compat.value("message").toString();
}

void AppController::applyCustomFfmpegDirToEnvironment()
{
    static QString s_originalPath;
    static QString s_appliedCustomDir;
    if (s_originalPath.isNull()) {
        s_originalPath = qEnvironmentVariable("PATH");
    }
    if (m_customFfmpegDir == s_appliedCustomDir) {
        // PATH already reflects this setting -- skip qputenv. Rewriting the
        // process-wide environment on every construction (even as a no-op)
        // races with ProbeEngine::recheckBackground()'s worker threads, which
        // read that same environment via QProcess::start. Constructing many
        // AppControllers back to back (as the test suite does) turns that
        // race into a near-guaranteed crash on Windows.
        return;
    }
    s_appliedCustomDir = m_customFfmpegDir;
    QString path = s_originalPath;
    if (!m_customFfmpegDir.isEmpty()) {
        const QChar sep = QDir::listSeparator();
        path = m_customFfmpegDir + sep + path;
    }
    qputenv("PATH", path.toLocal8Bit());
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

qint64 AppController::snapToKeyframe(qint64 ms)
{
    if (m_keyframes.isEmpty()) {
        return ms;
    }
    const QJsonObject snap = RustBridge::nearestKeyframe(keyframesJson(), ms);
    if (!snap.value("ok").toBool()) {
        return ms;
    }
    const qint64 snapped = static_cast<qint64>(snap.value("snap").toObject().value("snapped_ms").toDouble());
    const auto it = std::lower_bound(m_keyframes.begin(), m_keyframes.end(), snapped);
    if (it != m_keyframes.end() && *it == snapped) {
        m_currentIndex = static_cast<int>(std::distance(m_keyframes.begin(), it));
        emit keyframesChanged();
    }
    return snapped;
}

void AppController::dismissNotice()
{
    setNotice(QString(), QString());
}

bool AppController::outputExists() const
{
    return !m_outputPath.isEmpty() && QFileInfo::exists(m_outputPath);
}

void AppController::setNotice(const QString &text, const QString &tone)
{
    if (m_noticeText == text && m_noticeTone == tone) {
        return;
    }
    m_noticeText = text;
    m_noticeTone = tone;
    emit noticeChanged();
}

void AppController::setProbing(bool probing)
{
    if (m_probing == probing) {
        return;
    }
    m_probing = probing;
    emit probingChanged();
}

QString AppController::ensureThumbDir()
{
    if (!m_thumbDirPath.isEmpty()) {
        return m_thumbDirPath;
    }
    const QString base = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (base.isEmpty()) {
        return QString();
    }
    QDir dir(base);
    const QString sub = QStringLiteral("seder-rewrap-thumbs-%1").arg(QCoreApplication::applicationPid());
    if (!dir.exists(sub) && !dir.mkpath(sub)) {
        return QString();
    }
    m_thumbDirPath = dir.filePath(sub);
    return m_thumbDirPath;
}

void AppController::generateThumbnail(qint64 timeMs, bool isIn)
{
    if (!m_probeEngine->ffmpegReady() || m_sourcePath.isEmpty()) {
        return;
    }
    const QString thumbDir = ensureThumbDir();
    if (thumbDir.isEmpty()) {
        return;
    }
    const QString source = m_sourcePath;
    const QString outFile = QStringLiteral("%1/%2-%3.png")
                                .arg(thumbDir, isIn ? QStringLiteral("in") : QStringLiteral("out"))
                                .arg(timeMs);

    const QJsonObject reply = RustBridge::ffmpegThumbnailCommand(source, timeMs, outFile);
    if (!reply.value("ok").toBool()) {
        return;
    }
    const RustBridge::Command command = RustBridge::commandFromJson(reply.value("command").toObject());

    QPointer<AppController> self(this);
    QThread *worker = QThread::create([self, command, source, timeMs, outFile, isIn] {
        const ProcessResult result = ProcessUtil::runCommand(command.program, command.args);
        if (!self) {
            return;
        }
        QMetaObject::invokeMethod(self.data(), [self, result, source, timeMs, outFile, isIn] {
            if (!self) {
                return;
            }
            // Staleness guard: drop the frame if the source changed or the
            // marker moved while ffmpeg was decoding.
            if (self->m_sourcePath != source) {
                return;
            }
            if (isIn && (!self->m_hasPendingIn || self->m_pendingIn != timeMs)) {
                return;
            }
            if (!isIn && (!self->m_hasPendingOut || self->m_pendingOut != timeMs)) {
                return;
            }
            if (!result.ok || !QFileInfo::exists(outFile)) {
                return;
            }
            const QUrl url = QUrl::fromLocalFile(outFile);
            if (isIn) {
                self->m_inThumbFile = outFile;
                self->m_inThumbUrl = url;
            } else {
                self->m_outThumbFile = outFile;
                self->m_outThumbUrl = url;
            }
            emit self->thumbnailsChanged();
        }, Qt::QueuedConnection);
    });
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}
