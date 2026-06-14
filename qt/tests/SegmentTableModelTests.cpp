#include "SegmentTableModel.h"

#include <QByteArray>
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

    void settersEmitRolesAndRefreshJson()
    {
        SegmentTableModel model;
        model.append(SegmentRow { "A", 0, 1000, "old", true });

        QSignalSpy changedSpy(&model, &SegmentTableModel::dataChanged);

        model.setName(0, "Renamed");
        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("Renamed"));
        QCOMPARE(changedSpy.size(), 1);

        model.setNotes(0, "fresh");
        QCOMPARE(model.data(model.index(0, 5)).toString(), QString("fresh"));
        QCOMPARE(changedSpy.size(), 2);

        model.setBounds(0, 2000, 6000);
        QCOMPARE(model.data(model.index(0, 2)).toString(), QString("00:00:02.000"));
        QCOMPARE(model.data(model.index(0, 3)).toString(), QString("00:00:06.000"));
        QCOMPARE(model.data(model.index(0, 4)).toString(), QString("00:00:04.000"));
        QCOMPARE(changedSpy.size(), 3);

        // The serialized JSON cache must reflect the edits.
        const QJsonObject serialized = model.toJsonArray().at(0).toObject();
        QCOMPARE(serialized.value("name").toString(), QString("Renamed"));
        QCOMPARE(serialized.value("notes").toString(), QString("fresh"));
        QCOMPARE(serialized.value("in_ms").toDouble(), 2000.0);
        QCOMPARE(serialized.value("out_ms").toDouble(), 6000.0);

        // No-op writes must not emit.
        model.setName(0, "Renamed");
        model.setNotes(0, "fresh");
        model.setBounds(0, 2000, 6000);
        QCOMPARE(changedSpy.size(), 3);
    }

    void moveDown_shiftsRowDownByOne()
    {
        SegmentTableModel model;
        model.append(SegmentRow { "A", 0, 1000, "", true });
        model.append(SegmentRow { "B", 1000, 2000, "", true });
        model.moveDown(0);
        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("B"));
        QCOMPARE(model.data(model.index(1, 1)).toString(), QString("A"));
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
};

QTEST_MAIN(SegmentTableModelTests)

#include "SegmentTableModelTests.moc"
