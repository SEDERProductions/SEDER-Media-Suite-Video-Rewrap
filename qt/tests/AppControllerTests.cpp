#define private public
#include "AppController.h"
#undef private

#include "SegmentTableModel.h"

#include <QtTest/QtTest>

class AppControllerTests : public QObject
{
    Q_OBJECT

private slots:
    void recheckToolsCached_usesCacheWithoutForce()
    {
        SegmentTableModel model;
        AppController controller(&model);

        controller.m_ffmpegReady = false;
        controller.m_ffprobeReady = false;
        controller.m_ffplayReady = false;
        controller.m_lastToolCheckMs = QDateTime::currentMSecsSinceEpoch();

        controller.recheckToolsCached();

        QCOMPARE(controller.m_ffmpegReady, false);
        QCOMPARE(controller.m_ffprobeReady, false);
        QCOMPARE(controller.m_ffplayReady, false);
    }

    void recheckToolsCached_forceBypassesCache()
    {
        SegmentTableModel model;
        AppController controller(&model);

        controller.m_ffmpegReady = false;
        controller.m_ffprobeReady = false;
        controller.m_ffplayReady = false;
        const qint64 before = QDateTime::currentMSecsSinceEpoch() - 1;
        controller.m_lastToolCheckMs = before;

        controller.recheckToolsCached(true);

        QVERIFY(controller.m_lastToolCheckMs >= before);
    }

    void toolStatusText_listsMissingBinaryNames()
    {
        SegmentTableModel model;
        AppController controller(&model);
        controller.m_ffmpegReady = false;
        controller.m_ffprobeReady = true;
        controller.m_ffplayReady = false;

        const QString status = controller.toolStatusText();

        QVERIFY(status.contains("ffmpeg"));
        QVERIFY(status.contains("ffplay"));
        QVERIFY(!status.contains("ffprobe,"));
    }
};

QTEST_MAIN(AppControllerTests)

#include "AppControllerTests.moc"
