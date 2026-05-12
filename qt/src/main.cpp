#include "AppController.h"
#include "SegmentTableModel.h"

#include <QGuiApplication>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName("Seder Productions");
    QGuiApplication::setOrganizationDomain("sederproductions.com");
    QGuiApplication::setApplicationName("SEDER Media Suite Video Rewrap");
    QQuickStyle::setStyle("Fusion");

    SegmentTableModel segmentModel;
    AppController controller(&segmentModel);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("app", &controller);
    engine.rootContext()->setContextProperty("segmentModel", &segmentModel);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        [] { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/Main.qml")));

    return app.exec();
}
