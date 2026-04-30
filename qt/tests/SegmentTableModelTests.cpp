#include "SegmentTableModel.h"

#include <QJsonArray>
#include <QJsonObject>
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
};

QTEST_MAIN(SegmentTableModelTests)

#include "SegmentTableModelTests.moc"
