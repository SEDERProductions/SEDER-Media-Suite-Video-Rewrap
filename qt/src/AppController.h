#pragma once

#include "RustBridge.h"
#include "SegmentTableModel.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QUrl>
#include <QVariantList>
#include <atomic>
#include <functional>

class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(SegmentTableModel *segmentModel READ segmentModel CONSTANT)
    Q_PROPERTY(QString sourcePath READ sourcePath NOTIFY sourcePathChanged)
    Q_PROPERTY(QString outputPath READ outputPath NOTIFY outputPathChanged)
    Q_PROPERTY(QString logText READ logText NOTIFY logTextChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(bool ffmpegReady READ ffmpegReady NOTIFY toolsChanged)
    Q_PROPERTY(bool ffprobeReady READ ffprobeReady NOTIFY toolsChanged)
    Q_PROPERTY(bool ffplayReady READ ffplayReady NOTIFY toolsChanged)
    Q_PROPERTY(QString mediaFilename READ mediaFilename NOTIFY metadataChanged)
    Q_PROPERTY(QString durationText READ durationText NOTIFY metadataChanged)
    Q_PROPERTY(QString resolutionText READ resolutionText NOTIFY metadataChanged)
    Q_PROPERTY(QString codecText READ codecText NOTIFY metadataChanged)
    Q_PROPERTY(QString sizeText READ sizeText NOTIFY metadataChanged)
    Q_PROPERTY(QString mediaSummary READ mediaSummary NOTIFY metadataChanged)
    Q_PROPERTY(int keyframeCount READ keyframeCount NOTIFY keyframesChanged)
    Q_PROPERTY(QString currentKeyframeText READ currentKeyframeText NOTIFY keyframesChanged)
    Q_PROPERTY(QString pendingInText READ pendingInText NOTIFY markersChanged)
    Q_PROPERTY(QString pendingOutText READ pendingOutText NOTIFY markersChanged)
    Q_PROPERTY(QString totalDurationText READ totalDurationText NOTIFY segmentsChanged)
    Q_PROPERTY(int selectedRow READ selectedRow NOTIFY selectedRowChanged)
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(bool darkMode READ darkMode NOTIFY themeChanged)
    Q_PROPERTY(QUrl videoUrl READ videoUrl NOTIFY sourcePathChanged)
    Q_PROPERTY(qint64 durationMs READ durationMs NOTIFY metadataChanged)
    Q_PROPERTY(QVariantList keyframesMs READ keyframesMs NOTIFY keyframesChanged)
    Q_PROPERTY(qint64 currentKeyframeMs READ currentKeyframeMs NOTIFY keyframesChanged)
    Q_PROPERTY(qint64 inMs READ inMs NOTIFY markersChanged)
    Q_PROPERTY(qint64 outMs READ outMs NOTIFY markersChanged)
    Q_PROPERTY(QUrl inThumbUrl READ inThumbUrl NOTIFY thumbnailsChanged)
    Q_PROPERTY(QUrl outThumbUrl READ outThumbUrl NOTIFY thumbnailsChanged)
    Q_PROPERTY(QString noticeText READ noticeText NOTIFY noticeChanged)
    Q_PROPERTY(QString noticeTone READ noticeTone NOTIFY noticeChanged)
    Q_PROPERTY(bool probing READ probing NOTIFY probingChanged)

public:
    explicit AppController(SegmentTableModel *segments, QObject *parent = nullptr);

    SegmentTableModel *segmentModel() const;
    QString sourcePath() const;
    QString outputPath() const;
    QString logText() const;
    bool busy() const;
    double progress() const;
    bool ffmpegReady() const;
    bool ffprobeReady() const;
    bool ffplayReady() const;
    QString mediaFilename() const;
    QString durationText() const;
    QString resolutionText() const;
    QString codecText() const;
    QString sizeText() const;
    QString mediaSummary() const;
    int keyframeCount() const;
    QString currentKeyframeText() const;
    QString pendingInText() const;
    QString pendingOutText() const;
    QString totalDurationText() const;
    int selectedRow() const;
    QString theme() const;
    bool darkMode() const;
    QUrl videoUrl() const;
    qint64 durationMs() const;
    QVariantList keyframesMs() const;
    qint64 currentKeyframeMs() const;
    qint64 inMs() const;
    qint64 outMs() const;
    QUrl inThumbUrl() const;
    QUrl outThumbUrl() const;
    QString noticeText() const;
    QString noticeTone() const;
    bool probing() const;

    Q_INVOKABLE void openSource();
    Q_INVOKABLE void chooseOutput();
    Q_INVOKABLE void recheckTools();
    Q_INVOKABLE void saveProject();
    Q_INVOKABLE void loadProject();
    Q_INVOKABLE void exportTxtReport();
    Q_INVOKABLE void exportCsvReport();
    Q_INVOKABLE void startExport();
    Q_INVOKABLE void cancelExport();
    Q_INVOKABLE void jumpToTimecode(const QString &timecode);
    Q_INVOKABLE void previousKeyframe();
    Q_INVOKABLE void nextKeyframe();
    Q_INVOKABLE void previewCurrent();
    Q_INVOKABLE void setIn();
    Q_INVOKABLE void setOut();
    Q_INVOKABLE void addSegment(const QString &name, const QString &notes);
    Q_INVOKABLE void selectSegment(int row);
    Q_INVOKABLE void removeSegment(int row);
    Q_INVOKABLE void duplicateSegment(int row);
    Q_INVOKABLE void moveSegmentUp(int row);
    Q_INVOKABLE void moveSegmentDown(int row);
    Q_INVOKABLE void toggleSegment(int row, bool enabled);
    Q_INVOKABLE void setTheme(const QString &theme);
    Q_INVOKABLE qint64 snapToKeyframe(qint64 ms);
    Q_INVOKABLE void openDroppedFile(const QUrl &url);
    Q_INVOKABLE void dismissNotice();
    Q_INVOKABLE bool outputExists() const;

signals:
    void sourcePathChanged();
    void outputPathChanged();
    void logTextChanged();
    void busyChanged();
    void progressChanged();
    void toolsChanged();
    void metadataChanged();
    void keyframesChanged();
    void markersChanged();
    void segmentsChanged();
    void selectedRowChanged();
    void themeChanged();
    void thumbnailsChanged();
    void noticeChanged();
    void probingChanged();

private:
    struct ProcessResult {
        bool ok = false;
        QString stdoutText;
        QString stderrText;
        int exitCode = -1;
    };

    void setSourcePath(const QString &path);
    void setOutputPath(const QString &path);
    void setLogText(const QString &text);
    void setBusy(bool busy);
    void setProgress(double progress);
    void setSelectedRowValue(int row);
    void probeSource(const QString &path);
    void clearMediaState();
    void applyMetadata(const QJsonObject &metadata, const QJsonArray &keyframes);
    void updateTotalDuration();
    static QString formatMs(qint64 milliseconds);
    QString displayPath(const QString &path) const;
    QString toolStatusText() const;
    QJsonArray keyframesJson() const;
    static bool programExists(const QString &program);
    static ProcessResult runCommand(const RustBridge::Command &command, std::atomic_bool *cancel = nullptr);
    static QString bytesText(quint64 bytes);
    bool ensureCanExport();
    bool writeTextFile(const QString &path, const QString &contents);
    bool runExportFlow(const QString &dialogTitle,
        const QString &dialogFilter,
        const QString &defaultFilename,
        const std::function<QJsonObject()> &bridgeCall,
        const QString &payloadKey,
        const QString &cancelLog,
        const QString &successLog);
    void recheckToolsCached();
    void recheckToolsBackground();
    void setNotice(const QString &text, const QString &tone);
    void setError(const QString &text);
    void setProbing(bool probing);
    void generateThumbnail(qint64 timeMs, bool isIn);
    QString ensureThumbDir();

    SegmentTableModel *m_segments = nullptr;
    QString m_sourcePath;
    QString m_outputPath;
    QString m_logText;
    bool m_busy = false;
    double m_progress = 0.0;
    bool m_ffmpegReady = false;
    bool m_ffprobeReady = false;
    bool m_ffplayReady = false;
    QString m_mediaFilename = "No source";
    QString m_durationText = "N/A";
    QString m_resolutionText = "N/A";
    QString m_codecText = "N/A";
    QString m_sizeText = "N/A";
    QString m_mediaSummary = "Open a video to inspect stream-copy-safe keyframes.";
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
    std::atomic_bool m_cancelExport = false;
    qint64 m_lastToolCheckMs = 0;
    QString m_noticeText;
    QString m_noticeTone;
    bool m_probing = false;
    QUrl m_inThumbUrl;
    QUrl m_outThumbUrl;
    QString m_inThumbFile;
    QString m_outThumbFile;
    QString m_thumbDirPath;
};
