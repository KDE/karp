/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>
*/

#include <QtGlobal>
#ifdef Q_OS_ANDROID
#include <QGuiApplication>
#else
#include <QApplication>
#endif

#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>

#include "deafed.h"
#include "version-deafed.h"
#include <KAboutData>
#include <KLocalizedContext>
#include <KLocalizedString>

#include "deafedconfig.h"

using namespace Qt::Literals::StringLiterals;

#ifdef Q_OS_ANDROID
Q_DECL_EXPORT
#endif
int main(int argc, char *argv[])
{
#ifdef Q_OS_ANDROID
    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle(QStringLiteral("org.kde.breeze"));
#else
    QApplication app(argc, argv);

    // Default to org.kde.desktop style unless the user forces another style
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(u"org.kde.desktop"_s);
    }
#endif

#ifdef Q_OS_WINDOWS
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }

    QApplication::setStyle(QStringLiteral("breeze"));
    auto font = app.font();
    font.setPointSize(10);
    app.setFont(font);
#endif

    KLocalizedString::setApplicationDomain("deafed");
    QCoreApplication::setOrganizationName(u"KDE"_s);

    KAboutData aboutData(
        // The program name used internally.
        u"deafed"_s,
        // A displayable program name string.
        i18nc("@title", "Deaf Ed"),
        // The program version string.
        QStringLiteral(DEAFED_VERSION_STRING),
        // Short description of what the app does.
        i18n("Simple PDF editor"),
        // The license this code is released under.
        KAboutLicense::GPL,
        // Copyright Statement.
        i18n("(c) 2024 Tomasz Bojczuk"));
    aboutData.addAuthor(i18nc("@info:credit", "Tomasz Bojczuk"),
                        i18nc("@info:credit", "Maintainer"),
                        u"seelook@gmail.com"_s,
                        u"https://sourceforge.net/u/seelook/profile"_s);
    // aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));
    KAboutData::setApplicationData(aboutData);
    QGuiApplication::setWindowIcon(QIcon::fromTheme(u"application-pdf"_s));
    // QGuiApplication::setWindowIcon(QIcon::fromTheme(u"org.kde.deafed"_s));

    QQmlApplicationEngine engine;

    auto config = deafedConfig::self();

    qmlRegisterSingletonInstance("org.kde.deafed.config", 1, 0, "DeafEdConf", config);

    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.loadFromModule("org.kde.deafed", u"Main"_s);

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
