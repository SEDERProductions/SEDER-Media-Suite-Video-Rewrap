#include "ProbeEngine.h"

#include "ProcessUtil.h"
#include "RustBridge.h"

#include <QDateTime>
#include <QFileInfo>
#include <QPointer>
#include <QThread>
#include <future>

ProbeEngine::ProbeEngine(QObject *parent)
    : QObject(parent)
{
}

void ProbeEngine::recheckTools()
{
    const bool ffmpeg = ProcessUtil::programExists("ffmpeg");
    const bool ffprobe = ProcessUtil::programExists("ffprobe");
    const bool ffplay = ProcessUtil::programExists("ffplay");
    m_ffmpegReady = ffmpeg;
    m_ffprobeReady = ffprobe;
    m_ffplayReady = ffplay;
    m_lastToolCheckMs = QDateTime::currentMSecsSinceEpoch();
    emit toolsChanged(m_ffmpegReady, m_ffprobeReady, m_ffplayReady);
}

void ProbeEngine::recheckCached()
{
    constexpr qint64 kToolCacheTtlMs = 5'000;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (m_lastToolCheckMs != 0 && now - m_lastToolCheckMs < kToolCacheTtlMs) {
        return;
    }
    recheckTools();
}

void ProbeEngine::recheckBackground()
{
    QPointer<ProbeEngine> self(this);
    QThread *worker = QThread::create([self] {
        const bool ffmpegOk = ProcessUtil::programExists("ffmpeg");
        const bool ffprobeOk = ProcessUtil::programExists("ffprobe");
        const bool ffplayOk = ProcessUtil::programExists("ffplay");
        const qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
        if (self) {
            QMetaObject::invokeMethod(self, [self, ffmpegOk, ffprobeOk, ffplayOk, timestamp] {
                if (!self) return;
                self->m_ffmpegReady = ffmpegOk;
                self->m_ffprobeReady = ffprobeOk;
                self->m_ffplayReady = ffplayOk;
                self->m_lastToolCheckMs = timestamp;
                emit self->toolsChanged(self->m_ffmpegReady, self->m_ffprobeReady, self->m_ffplayReady);
                emit self->logMessage(self->toolStatusText());
            }, Qt::QueuedConnection);
        }
    });
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

void ProbeEngine::probeSource(const QString &path)
{
    if (!m_ffprobeReady) {
        emit logMessage(toolStatusText());
        return;
    }

    QPointer<ProbeEngine> self(this);
    QThread *worker = QThread::create([self, path] {
        const QJsonObject metadataReply = RustBridge::ffprobeMetadataCommand(path);
        const QJsonObject keyframeReply = RustBridge::ffprobeKeyframeCommand(path);
        if (!metadataReply.value("ok").toBool() || !keyframeReply.value("ok").toBool()) {
            const QString error = metadataReply.value("error").toString(keyframeReply.value("error").toString());
            if (self) {
                QMetaObject::invokeMethod(self, [self, error] {
                    if (!self) return;
                    emit self->probeComplete(false, QJsonObject(), QJsonArray(), error);
                }, Qt::QueuedConnection);
            }
            return;
        }

        auto metadataFuture = std::async(std::launch::async, [&] {
            const RustBridge::Command cmd = RustBridge::commandFromJson(metadataReply.value("command").toObject());
            return ProcessUtil::runCommand(cmd.program, cmd.args);
        });
        auto keyframesFuture = std::async(std::launch::async, [&] {
            const RustBridge::Command cmd = RustBridge::commandFromJson(keyframeReply.value("command").toObject());
            return ProcessUtil::runCommand(cmd.program, cmd.args);
        });
        const ProcessResult metadata = metadataFuture.get();
        const ProcessResult keyframes = keyframesFuture.get();
        if (!metadata.ok || !keyframes.ok) {
            const QString error = !metadata.ok ? metadata.stderrText : keyframes.stderrText;
            if (self) {
                QMetaObject::invokeMethod(self, [self, error] {
                    if (!self) return;
                    emit self->probeComplete(false, QJsonObject(), QJsonArray(), error);
                }, Qt::QueuedConnection);
            }
            return;
        }

        const quint64 size = QFileInfo(path).size();
        const QJsonObject parsed = RustBridge::parseProbeResult(path, size, metadata.stdoutText, keyframes.stdoutText);
        if (self) {
            QMetaObject::invokeMethod(self, [self, parsed] {
                if (!self) return;
                if (!parsed.value("ok").toBool()) {
                    emit self->probeComplete(false, QJsonObject(), QJsonArray(), parsed.value("error").toString());
                    return;
                }
                emit self->probeComplete(true, parsed.value("metadata").toObject(), parsed.value("keyframes").toArray(), QString());
            }, Qt::QueuedConnection);
        }
    });
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

void ProbeEngine::clearState()
{
    m_metadataJson = QJsonObject();
    m_keyframes.clear();
}

QString ProbeEngine::toolStatusText() const
{
    QStringList missing;
    if (!m_ffmpegReady) missing.push_back("ffmpeg");
    if (!m_ffprobeReady) missing.push_back("ffprobe");
    if (!m_ffplayReady) missing.push_back("ffplay");
    if (missing.isEmpty()) {
        return "FFmpeg, FFprobe, and FFplay detected.";
    }
    return QStringLiteral("Missing required binaries: %1. Install FFmpeg from ffmpeg.org or with Homebrew: brew install ffmpeg.")
        .arg(missing.join(", "));
}
