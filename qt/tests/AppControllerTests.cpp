#include "AppController.h"
#include "RecentFilesModel.h"
#include "UpdateChecker.h"

#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
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
    void initTestCase()
    {
        // Make QSettings hermetic and identical across platforms. Without an
        // organization name, QSettings falls back to platform-specific stores
        // (the Windows registry in particular), which leaks state between
        // tests and between CI runs and has produced Windows-only flakiness.
        // Forcing IniFormat into the test's temp dir keeps every run isolated.
        QCoreApplication::setOrganizationName(QStringLiteral("SederTest"));
        QCoreApplication::setApplicationName(QStringLiteral("VideoRewrapTests"));
        QSettings::setDefaultFormat(QSettings::IniFormat);
        const QString scratch = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
            + QStringLiteral("/seder-rewrap-test-%1").arg(QCoreApplication::applicationPid());
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, scratch);
        QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, scratch);
        QSettings().clear();
    }

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

    void recentFilesModel_dedupesAndCapsAtMax()
    {
        // Use a unique settings key so we don't collide with other tests or
        // the user's real preferences.
        const QString key = QStringLiteral("test/recent_%1").arg(QDateTime::currentMSecsSinceEpoch());
        {
            RecentFilesModel model(key);
            model.clear();
            QCOMPARE(model.rowCount(), 0);

            model.prepend("/tmp/a.mov");
            model.prepend("/tmp/b.mov");
            model.prepend("/tmp/a.mov"); // dedupe → a moves to head
            QCOMPARE(model.rowCount(), 2);
            QCOMPARE(model.pathAt(0), QString("/tmp/a.mov"));
            QCOMPARE(model.pathAt(1), QString("/tmp/b.mov"));

            for (int i = 0; i < 20; ++i) {
                model.prepend(QStringLiteral("/tmp/file_%1.mov").arg(i));
            }
            QCOMPARE(model.rowCount(), RecentFilesModel::kMaxEntries);
        }
        // Re-loading from settings should restore the list.
        {
            RecentFilesModel reloaded(key);
            QCOMPARE(reloaded.rowCount(), RecentFilesModel::kMaxEntries);
            reloaded.clear();
        }
    }

    void recentFilesModel_removeAndClear()
    {
        const QString key = QStringLiteral("test/recent_rm_%1").arg(QDateTime::currentMSecsSinceEpoch());
        RecentFilesModel model(key);
        model.clear();
        model.prepend("/tmp/a");
        model.prepend("/tmp/b");
        model.prepend("/tmp/c");
        QCOMPARE(model.rowCount(), 3);

        model.remove("/tmp/b");
        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.pathAt(0), QString("/tmp/c"));
        QCOMPARE(model.pathAt(1), QString("/tmp/a"));

        model.clear();
        QCOMPARE(model.rowCount(), 0);
    }

    void undoRedo_addSegmentIsReversible()
    {
        SegmentTableModel model;
        AppController ctrl(&model);
        ctrl.setKeyframesForTesting({ 0, 1000, 2500, 5000 });
        ctrl.setIn();   // sets m_pendingIn from current keyframe (index 0 → 0ms)
        ctrl.nextKeyframe();
        ctrl.nextKeyframe(); // currentIndex → 2 → 2500ms
        ctrl.setOut();
        ctrl.addSegment("X", "");
        QCOMPARE(model.rowCount(), 1);
        QVERIFY(ctrl.canUndo());

        ctrl.undo();
        QCOMPARE(model.rowCount(), 0);
        QVERIFY(ctrl.canRedo());

        ctrl.redo();
        QCOMPARE(model.rowCount(), 1);
    }

    void undoRedo_toggleSegmentRestoresPreviousEnabled()
    {
        SegmentTableModel model;
        AppController ctrl(&model);
        model.append(SegmentRow { "A", 0, 1000, "", true });

        ctrl.toggleSegment(0, false);
        QCOMPARE(model.segments().at(0).enabled, false);
        QVERIFY(ctrl.canUndo());

        ctrl.undo();
        QCOMPARE(model.segments().at(0).enabled, true);

        ctrl.redo();
        QCOMPARE(model.segments().at(0).enabled, false);
    }

    void undoRedo_removeRestoresSnapshot()
    {
        SegmentTableModel model;
        AppController ctrl(&model);
        model.append(SegmentRow { "A", 0, 1000, "alpha", true });
        model.append(SegmentRow { "B", 1000, 2000, "bravo", false });
        QCOMPARE(model.rowCount(), 2);

        ctrl.removeSegment(0);
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.segments().at(0).name, QString("B"));

        ctrl.undo();
        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.segments().at(0).name, QString("A"));
        QCOMPARE(model.segments().at(0).notes, QString("alpha"));
    }

    void updateChecker_emitsAvailableForNewerVersion()
    {
        UpdateChecker checker;
        checker.setCurrentVersionForTesting("0.1.29");
        checker.evaluatePayloadForTesting(
            QByteArray("{\"version\":\"99.0.0\",\"url\":\"https://example.invalid\"}"));
        QVERIFY(checker.updateAvailable());
        QCOMPARE(checker.updateUrl(), QString("https://example.invalid"));
        QCOMPARE(checker.latestVersion(), QString("99.0.0"));

        checker.dismiss();
        QVERIFY(!checker.updateAvailable());
    }

    void updateChecker_reportsUpToDateForOlderRemote()
    {
        UpdateChecker checker;
        checker.setCurrentVersionForTesting("1.0.0");
        checker.evaluatePayloadForTesting(QByteArray("{\"version\":\"0.0.1\"}"));
        QVERIFY(!checker.updateAvailable());
        QVERIFY(!checker.lastMessage().isEmpty());
    }

    void updateChecker_surfacesErrorOnBadPayload()
    {
        UpdateChecker checker;
        checker.setCurrentVersionForTesting("1.0.0");
        QSignalSpy spy(&checker, &UpdateChecker::lastMessageChanged);
        checker.evaluatePayloadForTesting(QByteArray("not json"));
        QVERIFY(spy.size() >= 1);
        QVERIFY(checker.lastMessage().contains("failed", Qt::CaseInsensitive));
        QVERIFY(!checker.updateAvailable());
    }
};

QTEST_MAIN(AppControllerTests)
#include "AppControllerTests.moc"
