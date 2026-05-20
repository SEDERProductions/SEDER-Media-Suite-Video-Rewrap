#include "AppController.h"
#include "SegmentTableModel.h"
#include "seder_version.h"

#include <QApplication>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QStringList>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("Seder Productions"));
    QApplication::setOrganizationDomain(QStringLiteral("sederproductions.com"));
    QApplication::setApplicationName(QStringLiteral("SEDER Media Suite Video Rewrap"));
    QApplication::setApplicationVersion(QStringLiteral(SEDER_APP_VERSION));
    QQuickStyle::setStyle(QStringLiteral("Fusion"));

    // CLI: --no-update-check disables the launch-time update probe for THIS
    // run only (without touching the persisted preference). The flag also
    // works for CI smoke runs and offline builds. AppController reads the
    // env var below in its constructor.
    const QStringList args = QApplication::arguments();
    if (args.contains(QStringLiteral("--no-update-check"))) {
        qputenv("SEDER_DISABLE_UPDATE_CHECK", "1");
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
