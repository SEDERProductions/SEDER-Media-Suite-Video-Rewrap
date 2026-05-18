#include "AppController.h"
#include "SegmentTableModel.h"

#include <QApplication>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QSettings>
#include <QStringList>

#ifndef SEDER_APP_VERSION
#define SEDER_APP_VERSION "0.0.0"
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("Seder Productions"));
    QApplication::setOrganizationDomain(QStringLiteral("sederproductions.com"));
    QApplication::setApplicationName(QStringLiteral("SEDER Media Suite Video Rewrap"));
    QApplication::setApplicationVersion(QStringLiteral(SEDER_APP_VERSION));
    QQuickStyle::setStyle(QStringLiteral("Fusion"));

    // CLI: --no-update-check disables the launch-time update probe without
    // touching the persisted preference. Useful for CI smoke runs and for
    // offline builds.
    const QStringList args = QApplication::arguments();
    if (args.contains(QStringLiteral("--no-update-check"))) {
        QSettings().setValue("update/checkOnLaunch", false);
    }

    SegmentTableModel segmentModel;
    AppController controller(&segmentModel);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("app"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("segmentModel"), &segmentModel);

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        [] { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule(QStringLiteral("Seder.VideoRewrap"), QStringLiteral("Main"));
#else
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [](QObject *obj, const QUrl &) {
            if (!obj) QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/Seder/VideoRewrap/qml/Main.qml")));
#endif

    return app.exec();
}
