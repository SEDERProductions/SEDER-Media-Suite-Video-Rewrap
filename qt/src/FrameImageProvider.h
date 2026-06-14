#pragma once

#include "FrameGrabber.h"

#include <QQuickImageProvider>

// QML image provider adapter for the program monitor. Kept separate from
// FrameGrabber so the grabber (the testable logic) needs only Qt Gui/Core
// and the test binaries don't link — or load — Qt Quick. Header-only; the
// app target pulls it in via main.cpp.
class FrameImageProvider : public QQuickImageProvider
{
public:
    explicit FrameImageProvider(FrameGrabber *grabber)
        : QQuickImageProvider(QQuickImageProvider::Image)
        , m_grabber(grabber)
    {
    }

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override
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

private:
    FrameGrabber *m_grabber;
};
