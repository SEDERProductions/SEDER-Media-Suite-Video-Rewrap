#pragma once

#include <QHash>
#include <QImage>
#include <QList>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

class QProcess;

// Extracts single still frames at the playhead with ffmpeg (BMP over
// image2pipe — no temp files) for the program monitor. Scrubbing is
// debounced; an in-flight grab is killed and replaced by the newest
// request. Recently shown frames are kept in a small LRU cache so
// keyframe stepping back and forth is instant.
class FrameGrabber : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int frameSerial READ frameSerial NOTIFY frameChanged)
    Q_PROPERTY(bool hasFrame READ hasFrame NOTIFY frameChanged)
    Q_PROPERTY(bool grabbing READ grabbing NOTIFY grabbingChanged)

public:
    explicit FrameGrabber(QObject *parent = nullptr);

    int frameSerial() const { return m_serial; }
    bool hasFrame() const;
    bool grabbing() const { return m_grabbing; }

    void requestFrame(const QString &source, qint64 positionMs);
    void clear();

    // Thread-safe copy for the QML image provider (called off the GUI
    // thread by Quick's pixmap reader).
    QImage frameForProvider() const;

    // Pure command construction, unit-tested.
    static QStringList grabArguments(const QString &source, qint64 positionMs, int maxWidth = 1280);

signals:
    void frameChanged();
    void grabbingChanged();

private:
    void onDebounceTimeout();
    void startGrab();
    void onProcessFinished();
    void publish(qint64 positionMs, const QImage &image);
    void setGrabbing(bool grabbing);

    QTimer m_debounce;
    QProcess *m_process = nullptr;
    bool m_grabbing = false;
    bool m_restartAfterFinish = false;

    QString m_pendingSource;
    qint64 m_pendingMs = -1;
    qint64 m_inFlightMs = -1;

    mutable QMutex m_imageMutex;
    QImage m_image;
    int m_serial = 0;

    QString m_cacheSource;
    QHash<qint64, QImage> m_cache;
    QList<qint64> m_cacheOrder;
};
