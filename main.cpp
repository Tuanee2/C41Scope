#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtQml>

#include "src/demo/fake_data_generator.h"
#include "src/scope_controller.h"
#include "src/scope_view.h"

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<c41scope::ScopeView>("C41Scope", 1, 0, "ScopeView");

    c41scope::ScopeController scopeController;
    c41scope::FakeDataGenerator fakeGenerator;
    fakeGenerator.setController(&scopeController);
    fakeGenerator.start();

    QObject::connect(&app, &QCoreApplication::aboutToQuit, &fakeGenerator, [&fakeGenerator]() {
        fakeGenerator.stop();
    });

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("scopeController", &scopeController);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("C41Scope", "Main");

    return app.exec();
}

