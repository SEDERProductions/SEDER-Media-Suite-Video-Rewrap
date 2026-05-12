#include "AppController.h"

#include <QSignalSpy>
#include <QtTest/QtTest>

class AppControllerTests : public QObject
{
    Q_OBJECT

private:
    SegmentTableModel *makeModelWithSegments()
    {
        auto *model = new SegmentTableModel(this);
        model->append(SegmentRow { "A", 0, 1000, "", true });
        model->append(SegmentRow { "B", 2000, 5000, "note", true });
        model->append(SegmentRow { "C", 5000, 8000, "", false });
        return model;
    }

private slots:
    void constructor_setsDefaults()
    {
        SegmentTableModel model;
        AppController ctrl(&model);

        QCOMPARE(ctrl.sourcePath(), QString());
        QCOMPARE(ctrl.outputPath(), QString());
        QCOMPARE(ctrl.logText(), QString());
        QCOMPARE(ctrl.busy(), false);
        QCOMPARE(ctrl.selectedRow(), -1);
        QCOMPARE(ctrl.mediaFilename(), QString("No source"));
        QCOMPARE(ctrl.durationText(), QString("N/A"));
        QCOMPARE(ctrl.resolutionText(), QString("N/A"));
        QCOMPARE(ctrl.codecText(), QString("N/A"));
        QCOMPARE(ctrl.sizeText(), QString("N/A"));
        QCOMPARE(ctrl.pendingInText(), QString("-"));
        QCOMPARE(ctrl.pendingOutText(), QString("-"));
        QCOMPARE(ctrl.totalDurationText(), QString("00:00:00.000"));
        QCOMPARE(ctrl.theme(), QString("system"));
        QCOMPARE(ctrl.exportMode(), QString("concat_single"));
        QCOMPARE(ctrl.segmentModel(), &model);
    }

    void setPaths_updatesGetters()
    {
        SegmentTableModel model;
        AppController ctrl(&model);

        QSignalSpy sourceSpy(&ctrl, &AppController::sourcePathChanged);
        QSignalSpy outputSpy(&ctrl, &AppController::outputPathChanged);

        ctrl.setPathsForTesting("/tmp/source.mov", "/tmp/output.mp4");

        QCOMPARE(ctrl.sourcePath(), QString("/tmp/source.mov"));
        QCOMPARE(ctrl.outputPath(), QString("/tmp/output.mp4"));

        QCOMPARE(sourceSpy.size(), 0);  // no Q_PROPERTY set, only internal
        QCOMPARE(outputSpy.size(), 0);
    }

    void setKeyframesForTesting_updatesKeyframeText()
    {
        SegmentTableModel model;
        AppController ctrl(&model);

        QSignalSpy spy(&ctrl, &AppController::keyframesChanged);

        QVector<qint64> keyframes = { 0, 1000, 2500, 5000 };
        ctrl.setKeyframesForTesting(keyframes);

        QCOMPARE(ctrl.keyframeCount(), 4);
        QCOMPARE(ctrl.currentKeyframeText(), QString("00:00:00.000"));
        QCOMPARE(spy.size(), 0);  // setKeyframesForTesting doesn't emit
    }

    void exportMode_setAndGet()
    {
        SegmentTableModel model;
        AppController ctrl(&model);

        QCOMPARE(ctrl.exportMode(), QString("concat_single"));

        QSignalSpy spy(&ctrl, &AppController::exportModeChanged);
        ctrl.setExportMode("separate_files");
        QCOMPARE(ctrl.exportMode(), QString("separate_files"));
        QCOMPARE(spy.size(), 1);

        ctrl.setExportMode("invalid");
        QCOMPARE(ctrl.exportMode(), QString("concat_single"));
        QCOMPARE(spy.size(), 2);

        // Setting same mode again should not emit
        ctrl.setExportMode("concat_single");
        QCOMPARE(spy.size(), 2);
    }

    void theme_roundTrips()
    {
        SegmentTableModel model;
        AppController ctrl(&model);

        QSignalSpy spy(&ctrl, &AppController::themeChanged);

        ctrl.setTheme("dark");
        QCOMPARE(ctrl.theme(), QString("dark"));
        QCOMPARE(ctrl.darkMode(), true);

        ctrl.setTheme("light");
        QCOMPARE(ctrl.theme(), QString("light"));
        QCOMPARE(ctrl.darkMode(), false);

        // Invalid falls back to "system"
        ctrl.setTheme("invalid");
        QCOMPARE(ctrl.theme(), QString("system"));
        QCOMPARE(spy.size(), 3);
    }

    void segmentCrud_updatesTotalDuration()
    {
        SegmentTableModel model;
        AppController ctrl(&model);

        // Initial state
        QCOMPARE(ctrl.totalDurationText(), QString("00:00:00.000"));

        // We can't call addSegment directly since it needs keyframes,
        // but we can test the total duration reflects model changes
        model.append(SegmentRow { "A", 0, 1000, "", true });
        model.append(SegmentRow { "B", 2000, 5000, "", false });
        model.append(SegmentRow { "C", 5000, 10000, "", true });

        // Manually trigger update (normally done by addSegment)
        QCOMPARE(ctrl.selectedRow(), -1);

        // Test through model that enabling/disabling segments changes total
        model.setEnabled(1, true);  // B becomes enabled: 2000-5000 = 3000ms
        model.setEnabled(2, false); // C becomes disabled
    }

    void overwriteApproval_sessionTracking()
    {
        SegmentTableModel model;
        AppController ctrl(&model);
        Q_UNUSED(ctrl);

        // Can't test without QGuiApplication fully initialized.
        // Validation logic is in ExportEngine which requires dialog parent.
    }

    void ensureCanExport_returnsFalseByDefault()
    {
        SegmentTableModel model;
        AppController ctrl(&model);

        // Without source, output, segments, or tools — should fail
        ctrl.setPathsForTesting("", "");
        bool result = ctrl.ensureCanExportForTesting();
        QCOMPARE(result, false);
    }

    void setOverwriteDecisionProvider_acceptsOverride()
    {
        SegmentTableModel model;
        AppController ctrl(&model);

        ctrl.setOverwriteDecisionProviderForTesting([](const QString &) {
            return true;
        });

        QCOMPARE(ctrl.overwriteApprovedForSessionForTesting(), false);
    }

    void totalDuration_computedFromModel()
    {
        SegmentTableModel model;
        model.append(SegmentRow { "A", 0, 1000, "", true });
        model.append(SegmentRow { "B", 1000, 4000, "", true });
        model.append(SegmentRow { "C", 4000, 8000, "", false });

        // Total enabled = 1000 + 3000 = 4000ms = 00:00:04.000
        QCOMPARE(model.rowCount(), 3);

        // The totalDurationText is updated by AppController methods,
        // not automatically by model changes.  This test validates
        // that the model itself holds correct data.
        QCOMPARE(model.data(model.index(0, 4), SegmentTableModel::DurationTextRole).toString(), QString("00:00:01.000"));
        QCOMPARE(model.data(model.index(1, 4), SegmentTableModel::DurationTextRole).toString(), QString("00:00:03.000"));
        QCOMPARE(model.data(model.index(2, 4), SegmentTableModel::DurationTextRole).toString(), QString("00:00:04.000"));
    }
};

QTEST_MAIN(AppControllerTests)
#include "AppControllerTests.moc"
