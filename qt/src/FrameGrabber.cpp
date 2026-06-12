#include "FrameGrabber.h"

#include <QProcess>

namespace {
constexpr int kDebounceMs = 80;
constexpr int kCacheCapacity = 24;
}

FrameGrabber::FrameGrabber(QObject *parent)
    : QObject(parent)
{
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(kDebounceMs);
    connect(&m_debounce, &QTimer::timeout, this, &FrameGrabber::onDebounceTimeout);
}

bool FrameGrabber::hasFrame() const
{
    QMutexLocker lock(&m_imageMutex);
    return !m_image.isNull();
}

QStringList FrameGrabber::grabArguments(const QString &source, qint64 positionMs, int maxWidth)
{
    // -ss before -i: fast keyframe seek, exact here because the playhead
    // is always parked on a keyframe. The comma inside min() must be
    // escaped for ffmpeg's filtergraph parser (QProcess passes args
    // literally, there is no shell).
    return {
        QStringLiteral("-hide_banner"),
        QStringLiteral("-loglevel"), QStringLiteral("error"),
        QStringLiteral("-ss"), QString::number(positionMs / 1000.0, 'f', 3),
        QStringLiteral("-i"), source,
        QStringLiteral("-frames:v"), QStringLiteral("1"),
        QStringLiteral("-vf"), QStringLiteral("scale=min(%1\\,iw):-2").arg(maxWidth),
        QStringLiteral("-f"), QStringLiteral("image2pipe"),
        QStringLiteral("-vcodec"), QStringLiteral("bmp"),
        QStringLiteral("-"),
    };
}

void FrameGrabber::requestFrame(const QString &source, qint64 positionMs)
{
    if (source.isEmpty() || positionMs < 0) {
        clear();
        return;
    }

    if (source != m_cacheSource) {
        m_cache.clear();
        m_cacheOrder.clear();
        m_cacheSource = source;
    }

    const auto cached = m_cache.constFind(positionMs);
    if (cached != m_cache.constEnd()) {
        m_debounce.stop();
        m_pendingMs = -1;
        publish(positionMs, cached.value());
        return;
    }

    m_pendingSource = source;
    m_pendingMs = positionMs;
    m_debounce.start();
}

void FrameGrabber::clear()
{
    m_debounce.stop();
    m_pendingMs = -1;
    m_pendingSource.clear();
    m_cache.clear();
    m_cacheOrder.clear();
    m_cacheSource.clear();
    if (m_process) {
        m_restartAfterFinish = false;
        m_process->kill();
    }
    bool hadFrame = false;
    {
        QMutexLocker lock(&m_imageMutex);
        hadFrame = !m_image.isNull();
        m_image = QImage();
        ++m_serial;
    }
    if (hadFrame) {
        emit frameChanged();
    }
}

QImage FrameGrabber::frameForProvider() const
{
    QMutexLocker lock(&m_imageMutex);
    return m_image;
}

void FrameGrabber::onDebounceTimeout()
{
    if (m_pendingMs < 0) {
        return;
    }
    if (m_process) {
        // A grab is in flight; replace it with the newest request once
        // it dies.
        m_restartAfterFinish = true;
        m_process->kill();
        return;
    }
    startGrab();
}

void FrameGrabber::startGrab()
{
    if (m_pendingMs < 0 || m_pendingSource.isEmpty()) {
        return;
    }
    m_inFlightMs = m_pendingMs;
    const QString source = m_pendingSource;
    m_pendingMs = -1;

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);
    connect(m_process,
        static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
        this,
        [this](int, QProcess::ExitStatus) { onProcessFinished(); });
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            onProcessFinished();
        }
    });

    setGrabbing(true);
    m_process->start(QStringLiteral("ffmpeg"), grabArguments(source, m_inFlightMs));
}

void FrameGrabber::onProcessFinished()
{
    if (!m_process) {
        return;
    }
    QProcess *finished = m_process;
    m_process = nullptr;
    finished->deleteLater();

    const bool wasKilled = finished->exitStatus() != QProcess::NormalExit
        || finished->error() == QProcess::FailedToStart;
    const qint64 grabbedMs = m_inFlightMs;
    m_inFlightMs = -1;

    if (!wasKilled && finished->exitCode() == 0) {
        const QByteArray data = finished->readAllStandardOutput();
        QImage image = QImage::fromData(data, "BMP");
        if (!image.isNull()) {
            m_cache.insert(grabbedMs, image);
            m_cacheOrder.removeAll(grabbedMs);
            m_cacheOrder.append(grabbedMs);
            while (m_cacheOrder.size() > kCacheCapacity) {
                m_cache.remove(m_cacheOrder.takeFirst());
            }
            // Only publish if no newer request superseded this grab.
            if (m_pendingMs < 0) {
                publish(grabbedMs, image);
            }
        }
    }

    if (m_restartAfterFinish || m_pendingMs >= 0) {
        m_restartAfterFinish = false;
        startGrab();
        return;
    }
    setGrabbing(false);
}

void FrameGrabber::publish(qint64 positionMs, const QImage &image)
{
    Q_UNUSED(positionMs);
    {
        QMutexLocker lock(&m_imageMutex);
        m_image = image;
        ++m_serial;
    }
    emit frameChanged();
}

void FrameGrabber::setGrabbing(bool grabbing)
{
    if (m_grabbing == grabbing) {
        return;
    }
    m_grabbing = grabbing;
    emit grabbingChanged();
}

FrameImageProvider::FrameImageProvider(FrameGrabber *grabber)
    : QQuickImageProvider(QQuickImageProvider::Image)
    , m_grabber(grabber)
{
}

QImage FrameImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(id);
    Q_UNUSED(requestedSize);
    QImage image = m_grabber ? m_grabber->frameForProvider() : QImage();
    if (image.isNull()) {
        image = QImage(2, 2, QImage::Format_ARGB32);
        image.fill(Qt::transparent);
    }
    if (size) {
        *size = image.size();
    }
    return image;
}
