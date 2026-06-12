#pragma once

#include "ExportEngine.h"
#include "FrameGrabber.h"
#include "ProbeEngine.h"
#include "RecentFilesModel.h"
#include "RustBridge.h"
#include "SegmentCommands.h"
#include "SegmentTableModel.h"
#include "UpdateChecker.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QUndoStack>
#include <QVariant>
#include <QVector>
#include <functional>

class AppController : public QObject, public SegmentCommandContext
{
    Q_OBJECT
    Q_PROPERTY(SegmentTableModel *segmentModel READ segmentModel CONSTANT)
    Q_PROPERTY(RecentFilesModel *recentSources READ recentSources CONSTANT)
    Q_PROPERTY(RecentFilesModel *recentOutputs READ recentOutputs CONSTANT)
    Q_PROPERTY(RecentFilesModel *recentProjects READ recentProjects CONSTANT)
    Q_PROPERTY(UpdateChecker *updateChecker READ updateChecker CONSTANT)
    Q_PROPERTY(FrameGrabber *frameGrabber READ frameGrabber CONSTANT)
    Q_PROPERTY(QString sourcePath READ sourcePath NOTIFY sourcePathChanged)
    Q_PROPERTY(QString outputPath READ outputPath NOTIFY outputPathChanged)
    Q_PROPERTY(QString logText READ logText NOTIFY logTextChanged)
    Q_PROPERTY(QString lastErrorLog READ lastErrorLog NOTIFY lastErrorLogChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(bool ffmpegReady READ ffmpegReady NOTIFY toolsChanged)
    Q_PROPERTY(bool ffprobeReady READ ffprobeReady NOTIFY toolsChanged)
    Q_PROPERTY(bool ffplayReady READ ffplayReady NOTIFY toolsChanged)
    Q_PROPERTY(QString ffmpegVersionText READ ffmpegVersionText NOTIFY toolsChanged)
    Q_PROPERTY(bool ffmpegCompatible READ ffmpegCompatible NOTIFY toolsChanged)
    Q_PROPERTY(QString customFfmpegDir READ customFfmpegDir NOTIFY customFfmpegDirChanged)
    Q_PROPERTY(QString mediaFilename READ mediaFilename NOTIFY metadataChanged)
    Q_PROPERTY(QString durationText READ durationText NOTIFY metadataChanged)
    Q_PROPERTY(QString resolutionText READ resolutionText NOTIFY metadataChanged)
    Q_PROPERTY(QString codecText READ codecText NOTIFY metadataChanged)
    Q_PROPERTY(QString sizeText READ sizeText NOTIFY metadataChanged)
    Q_PROPERTY(QString mediaSummary READ mediaSummary NOTIFY metadataChanged)
    Q_PROPERTY(int keyframeCount READ keyframeCount NOTIFY keyframesChanged)
    Q_PROPERTY(QString currentKeyframeText READ currentKeyframeText NOTIFY positionMsChanged)
    Q_PROPERTY(qint64 durationMs READ durationMs NOTIFY metadataChanged)
    Q_PROPERTY(qint64 positionMs READ positionMs NOTIFY positionMsChanged)
    Q_PROPERTY(int currentKeyframeIndex READ currentKeyframeIndex NOTIFY positionMsChanged)
    Q_PROPERTY(QVariantList keyframesMs READ keyframesMs NOTIFY keyframesChanged)
    Q_PROPERTY(QString pendingInText READ pendingInText NOTIFY markersChanged)
    Q_PROPERTY(QString pendingOutText READ pendingOutText NOTIFY markersChanged)
    Q_PROPERTY(qint64 pendingInMs READ pendingInMs NOTIFY markersChanged)
    Q_PROPERTY(qint64 pendingOutMs READ pendingOutMs NOTIFY markersChanged)
    Q_PROPERTY(bool hasPendingIn READ hasPendingIn NOTIFY markersChanged)
    Q_PROPERTY(bool hasPendingOut READ hasPendingOut NOTIFY markersChanged)
    Q_PROPERTY(QString totalDurationText READ totalDurationText NOTIFY segmentsChanged)
    Q_PROPERTY(int selectedRow READ selectedRow NOTIFY selectedRowChanged)
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(bool darkMode READ darkMode NOTIFY themeChanged)
    Q_PROPERTY(QString exportMode READ exportMode WRITE setExportMode NOTIFY exportModeChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY undoStackChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY undoStackChanged)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)

public:
    explicit AppController(SegmentTableModel *segments, QObject *parent = nullptr);

    SegmentTableModel *segmentModel() const;
    RecentFilesModel *recentSources() const { return m_recentSources; }
    RecentFilesModel *recentOutputs() const { return m_recentOutputs; }
    RecentFilesModel *recentProjects() const { return m_recentProjects; }
    UpdateChecker *updateChecker() const { return m_updateChecker; }
    FrameGrabber *frameGrabber() const { return m_frameGrabber; }
    QString sourcePath() const;
    QString outputPath() const;
    QString logText() const;
    QString lastErrorLog() const { return m_lastErrorLog; }
    bool busy() const;
    double progress() const;
    bool ffmpegReady() const;
    bool ffprobeReady() const;
    bool ffplayReady() const;
    QString ffmpegVersionText() const { return m_ffmpegVersionText; }
    bool ffmpegCompatible() const { return m_ffmpegCompatible; }
    QString customFfmpegDir() const { return m_customFfmpegDir; }
    QString mediaFilename() const;
    QString durationText() const;
    QString resolutionText() const;
    QString codecText() const;
    QString sizeText() const;
    QString mediaSummary() const;
    int keyframeCount() const;
    QString currentKeyframeText() const;
    qint64 durationMs() const;
    qint64 positionMs() const;
    int currentKeyframeIndex() const;
    QVariantList keyframesMs() const;
    QString pendingInText() const;
    QString pendingOutText() const;
    qint64 pendingInMs() const { return m_hasPendingIn ? m_pendingIn : -1; }
    qint64 pendingOutMs() const { return m_hasPendingOut ? m_pendingOut : -1; }
    bool hasPendingIn() const { return m_hasPendingIn; }
    bool hasPendingOut() const { return m_hasPendingOut; }
    QString totalDurationText() const;
    int selectedRow() const;
    QString theme() const;
    bool darkMode() const;
    QString exportMode() const;
    bool canUndo() const { return m_undo.canUndo(); }
    bool canRedo() const { return m_undo.canRedo(); }
    QString appVersion() const;

    Q_INVOKABLE void openSource();
    Q_INVOKABLE void openSourcePath(const QString &path);
    Q_INVOKABLE void openProjectPath(const QString &path);
    Q_INVOKABLE void setOutputFromPath(const QString &path);
    Q_INVOKABLE void chooseOutput();
    Q_INVOKABLE void recheckTools();
    Q_INVOKABLE void saveProject();
    Q_INVOKABLE void loadProject();
    Q_INVOKABLE void exportTxtReport();
    Q_INVOKABLE void exportCsvReport();
    Q_INVOKABLE void startExport();
    Q_INVOKABLE void replaceFile();
    Q_INVOKABLE void cancelExport();
    Q_INVOKABLE void jumpToTimecode(const QString &timecode);
    Q_INVOKABLE void previousKeyframe();
    Q_INVOKABLE void nextKeyframe();
    Q_INVOKABLE void seekToMs(qint64 ms);
    Q_INVOKABLE qint64 nearestKeyframeMs(qint64 ms) const;
    Q_INVOKABLE void previewCurrent();
    Q_INVOKABLE void setIn();
    Q_INVOKABLE void setOut();
    Q_INVOKABLE void clearPendingMarkers();
    Q_INVOKABLE void setSegmentBounds(int row, qint64 inMs, qint64 outMs);
    Q_INVOKABLE void renameSegment(int row, const QString &name);
    Q_INVOKABLE void setSegmentNotes(int row, const QString &notes);
    Q_INVOKABLE void addSegment(const QString &name, const QString &notes);
    Q_INVOKABLE void selectSegment(int row);
    Q_INVOKABLE void removeSegment(int row);
    Q_INVOKABLE void duplicateSegment(int row);
    Q_INVOKABLE void moveSegmentUp(int row);
    Q_INVOKABLE void moveSegmentDown(int row);
    Q_INVOKABLE void toggleSegment(int row, bool enabled);
    Q_INVOKABLE void setTheme(const QString &theme);
    Q_INVOKABLE void setExportMode(const QString &mode);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void handleDroppedFile(const QString &url);
    Q_INVOKABLE void setCustomFfmpegDir(const QString &dir);
    Q_INVOKABLE void clearCustomFfmpegDir();
    Q_INVOKABLE void copyLastErrorLogToClipboard();

    bool ensureCanExportForTesting();
    void setPathsForTesting(const QString &source, const QString &output);
    void setToolsReadyForTesting(bool ffmpegReady, bool ffprobeReady);
    void setOverwriteDecisionProviderForTesting(const std::function<bool(const QString &)> &provider);
    bool overwriteApprovedForSessionForTesting() const;
    void setKeyframesForTesting(const QVector<qint64> &keyframes);
    QUndoStack *undoStackForTesting() { return &m_undo; }

    void onSegmentsMutated() override;

signals:
    void sourcePathChanged();
    void outputPathChanged();
    void logTextChanged();
    void lastErrorLogChanged();
    void busyChanged();
    void progressChanged();
    void toolsChanged();
    void customFfmpegDirChanged();
    void metadataChanged();
    void keyframesChanged();
    void positionMsChanged();
    void markersChanged();
    void segmentsChanged();
    void toastRequested(const QString &message, const QString &tone);
    void selectedRowChanged();
    void themeChanged();
    void exportModeChanged();
    void undoStackChanged();

private:
    void setSourcePath(const QString &path);
    void setOutputPath(const QString &path);
    void setLogText(const QString &text);
    void setLastErrorLog(const QString &text);
    void setSelectedRowValue(int row);
    void setCurrentIndex(int index);
    void updateTotalDuration();
    void clearMediaState();
    void applyMetadata(const QJsonObject &metadata, const QJsonArray &keyframes);
    QJsonArray keyframesJson() const;
    qint64 currentKeyframeMs() const;
    bool writeTextFile(const QString &path, const QString &contents);
    bool runExportFlow(const QString &dialogTitle,
        const QString &dialogFilter,
        const QString &defaultFilename,
        const std::function<QJsonObject()> &bridgeCall,
        const QString &payloadKey,
        const QString &cancelLog,
        const QString &successLog);
    void refreshFfmpegVersion();
    void applyCustomFfmpegDirToEnvironment();

    ProbeEngine *m_probeEngine = nullptr;
    ExportEngine *m_exportEngine = nullptr;
    SegmentTableModel *m_segments = nullptr;
    RecentFilesModel *m_recentSources = nullptr;
    RecentFilesModel *m_recentOutputs = nullptr;
    RecentFilesModel *m_recentProjects = nullptr;
    UpdateChecker *m_updateChecker = nullptr;
    FrameGrabber *m_frameGrabber = nullptr;
    QUndoStack m_undo;

    QString m_sourcePath;
    QString m_outputPath;
    QString m_logText;
    QString m_lastErrorLog;
    QString m_mediaFilename = tr("No source");
    QString m_durationText = tr("N/A");
    QString m_resolutionText = tr("N/A");
    QString m_codecText = tr("N/A");
    QString m_sizeText = tr("N/A");
    QString m_mediaSummary = tr("Open a video to inspect stream-copy-safe keyframes.");
    QString m_ffmpegVersionText;
    bool m_ffmpegCompatible = false;
    QString m_customFfmpegDir;
    QJsonObject m_metadataJson;
    QVector<qint64> m_keyframes;
    int m_currentIndex = 0;
    bool m_hasPendingIn = false;
    bool m_hasPendingOut = false;
    qint64 m_pendingIn = 0;
    qint64 m_pendingOut = 0;
    QString m_totalDurationText = "00:00:00.000";
    int m_selectedRow = -1;
    QString m_theme = "system";
    bool m_darkMode = true;
    QVector<qint64> m_previewPids;
};
