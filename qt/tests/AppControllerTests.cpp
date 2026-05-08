#define private public
#include "AppController.h"
#undef private

#include "SegmentTableModel.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QTemporaryDir>
#include <QFile>
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
        if (!qobject_cast<QGuiApplication *>(QCoreApplication::instance()))
            QSKIP("QGuiApplication is required for process probing");

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

    void existingOutput_declineCancelsExportGate()
    {
        SegmentTableModel model;
        AppController controller(&model);
        controller.setToolsReadyForTesting(true, true);

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString sourcePath = dir.filePath("source.mov");
        const QString outputPath = dir.filePath("output.mov");
        QVERIFY(QFile(sourcePath).open(QIODevice::WriteOnly));
        QVERIFY(QFile(outputPath).open(QIODevice::WriteOnly));

        model.append(SegmentRow { "A", 0, 1000, "", true });
        controller.setPathsForTesting(sourcePath, outputPath);
        controller.setOverwriteDecisionProviderForTesting([](const QString &) { return false; });

        QVERIFY(!controller.ensureCanExportForTesting());
        QVERIFY(!controller.overwriteApprovedForSessionForTesting());
        QVERIFY(controller.logText().contains("overwrite declined"));
    }

    void existingOutput_acceptAllowsExportGate()
    {
        SegmentTableModel model;
        AppController controller(&model);
        controller.setToolsReadyForTesting(true, true);

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString sourcePath = dir.filePath("source.mov");
        const QString outputPath = dir.filePath("output.mov");
        QVERIFY(QFile(sourcePath).open(QIODevice::WriteOnly));
        QVERIFY(QFile(outputPath).open(QIODevice::WriteOnly));

        model.append(SegmentRow { "A", 0, 1000, "", true });
        controller.setPathsForTesting(sourcePath, outputPath);
        controller.setOverwriteDecisionProviderForTesting([](const QString &) { return true; });
        controller.setKeyframesForTesting({0, 1000});

        const bool allowed = controller.ensureCanExportForTesting();
        QVERIFY2(allowed, qPrintable(controller.logText()));
        QVERIFY(controller.overwriteApprovedForSessionForTesting());
        QVERIFY(controller.logText().contains("Overwrite approved"));
    }
};

QTEST_MAIN(AppControllerTests)

#include "AppControllerTests.moc"
