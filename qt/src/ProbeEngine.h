#pragma once

#include <QJsonObject>
#include <QJsonArray>
#include <QObject>
#include <QString>
#include <QVector>
#include <atomic>
#include <functional>

class ProbeEngine : public QObject
{
    Q_OBJECT

public:
    explicit ProbeEngine(QObject *parent = nullptr);

    bool ffmpegReady() const { return m_ffmpegReady; }
    bool ffprobeReady() const { return m_ffprobeReady; }
    bool ffplayReady() const { return m_ffplayReady; }

    QJsonObject metadataJson() const { return m_metadataJson; }
    QVector<qint64> keyframes() const { return m_keyframes; }
    qint64 lastToolCheckMs() const { return m_lastToolCheckMs; }

    void recheckTools();
    void recheckCached();
    void recheckBackground();

    void probeSource(const QString &path);

    void clearState();

    QString toolStatusText() const;

signals:
    void toolsChanged(bool ffmpegReady, bool ffprobeReady, bool ffplayReady);
    void probeComplete(bool ok, QJsonObject metadata, QJsonArray keyframes, QString error);
    void logMessage(QString message);

private:
    bool m_ffmpegReady = false;
    bool m_ffprobeReady = false;
    bool m_ffplayReady = false;
    QJsonObject m_metadataJson;
    QVector<qint64> m_keyframes;
    qint64 m_lastToolCheckMs = 0;
    QString m_currentProbePath; // track current probe to ignore stale completions
};
