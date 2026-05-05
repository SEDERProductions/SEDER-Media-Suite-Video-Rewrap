#include "SegmentTableModel.h"
#define private public
#include "AppController.h"
#undef private

#include <QByteArray>
#include <QDateTime>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>
#include <QtTest/QtTest>

class SegmentTableModelTests : public QObject
{
    Q_OBJECT

private slots:
    void appendsAndSerializesRows()
    {
        SegmentTableModel model;
        model.append(SegmentRow { "A", 0, 1000, "keeper", true });

        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.columnCount(), 6);
        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("A"));
        QCOMPARE(model.data(model.index(0, 0), SegmentTableModel::EnabledRole).toBool(), true);

        const QJsonArray serialized = model.toJsonArray();
        QCOMPARE(serialized.size(), 1);
        QCOMPARE(serialized.at(0).toObject().value("name").toString(), QString("A"));
    }

    void togglesDuplicatesAndRemovesRows()
    {
        SegmentTableModel model;
        model.append(SegmentRow { "A", 0, 1000, "", true });

        model.setEnabled(0, false);
        QCOMPARE(model.data(model.index(0, 0)).toString(), QString("OFF"));

        model.duplicate(0);
        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.data(model.index(1, 1)).toString(), QString("A copy"));

        model.remove(0);
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("A copy"));
    }

    void loadsRowsFromJson()
    {
        SegmentTableModel model;
        model.fromJsonArray(QJsonArray {
            QJsonObject {
                { "name", "Loaded" },
                { "in_ms", 1000 },
                { "out_ms", 2500 },
                { "notes", "note" },
                { "enabled", true },
            },
        });

        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("Loaded"));
        QCOMPARE(model.data(model.index(0, 2), SegmentTableModel::InMsRole).toLongLong(), 1000);
    }

    // Pins the F1 fix: a child QProcess emitting > pipe-buffer-size of stdout
    // must not deadlock the parent. Mirrors AppController::runCommand's polling
    // loop pattern, draining stdout/stderr inside the loop.
    void runCommandPattern_drainsLargeStdoutWithoutDeadlock()
    {
#if defined(Q_OS_WIN)
        QSKIP("Pipe-buffer test requires a Unix shell with `dd`");
#else
        QProcess process;
        // Emit ~512 KiB to stdout — well past the typical 64 KiB pipe buffer.
        process.start("sh", { "-c", "dd if=/dev/zero bs=1024 count=512 2>/dev/null" });
        QVERIFY(process.waitForStarted(5000));

        QByteArray stdoutBuf;
        QByteArray stderrBuf;
        QElapsedTimer timer;
        timer.start();
        while (!process.waitForFinished(100)) {
            stdoutBuf += process.readAllStandardOutput();
            stderrBuf += process.readAllStandardError();
            if (timer.elapsed() > 10'000) {
                process.kill();
                QFAIL("runCommand pattern deadlocked on >64 KiB stdout");
            }
        }
        stdoutBuf += process.readAllStandardOutput();
        stderrBuf += process.readAllStandardError();

        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        QCOMPARE(process.exitCode(), 0);
        QCOMPARE(stdoutBuf.size(), 512 * 1024);
#endif
    }

    void toolStatusText_distinguishesExportAndPreviewReadiness()
    {
        SegmentTableModel model;
        AppController controller(&model);
        controller.m_ffmpegReady = true;
        controller.m_ffprobeReady = true;
        controller.m_ffplayReady = false;

        const QString status = controller.toolStatusText();
        QVERIFY(status.contains("Export Ready", Qt::CaseSensitive));
        QVERIFY(status.contains("Preview Limited", Qt::CaseSensitive));
        QVERIFY(!status.contains("Export Blocked", Qt::CaseSensitive));
    }

    void ensureCanExport_ignoresMissingFfplay()
    {
        SegmentTableModel model;
        model.append(SegmentRow { "A", 0, 1000, "", true });
        AppController controller(&model);

        controller.m_ffmpegReady = true;
        controller.m_ffprobeReady = true;
        controller.m_ffplayReady = false;
        controller.m_lastToolCheckMs = QDateTime::currentMSecsSinceEpoch();
        controller.setSourcePath("/tmp/source.mov");
        controller.setOutputPath("/tmp/out.mov");
        controller.m_keyframes = { 0, 1000, 2000 };

        QVERIFY(controller.ensureCanExport());
        QVERIFY(controller.logText().isEmpty() || !controller.logText().contains("missing ffplay", Qt::CaseInsensitive));
    }
};

QTEST_MAIN(SegmentTableModelTests)

#include "SegmentTableModelTests.moc"
