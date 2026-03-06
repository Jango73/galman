
/************************************************************************\

    Galman - Picture gallery manager
    Copyright (C) 2026 Jango73

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

\************************************************************************/

#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>

#include "ApplicationVersion.h"
#include "AppLogger.h"
#include "FavoritePairsManager.h"
#include "LanguageManager.h"
#include "ScriptEngine.h"
#include "ScriptManager.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    qInfo() << "Galman startup";
    app.setApplicationVersion(QStringLiteral(GALMAN_APPLICATION_VERSION));
    app.setWindowIcon(QIcon(":/Galman/qml/Assets/Galman.png"));
    AppLogger &appLogger = AppLogger::instance();
    appLogger.initialize();
    qInfo() << "Qt app initialized";

    QQmlApplicationEngine engine;
    ScriptEngine scriptEngine;
    ScriptManager scriptManager;
    LanguageManager languageManager(&engine);
    FavoritePairsManager favoritePairsManager;
    engine.rootContext()->setContextProperty("scriptEngine", &scriptEngine);
    engine.rootContext()->setContextProperty("scriptManager", &scriptManager);
    engine.rootContext()->setContextProperty("languageManager", &languageManager);
    engine.rootContext()->setContextProperty("favoritesManager", &favoritePairsManager);
    const QUrl url(u"qrc:/Galman/qml/App/Main.qml"_qs);
    qInfo() << "Loading QML root:" << url;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection);

    engine.load(url);
    qInfo() << "QML load complete. Root objects:" << engine.rootObjects().size();

    return app.exec();
}
