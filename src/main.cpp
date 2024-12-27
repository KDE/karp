/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>
*/

#ifdef Q_OS_ANDROID
#include <QGuiApplication>
#else
#include <QApplication>
#endif

#include <QIcon>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "karp_debug.h"
#include "version-karp.h"
#include <KAboutData>
#include <KCrash>
#include <KLocalizedContext>
#include <KLocalizedString>

#include "karpconfig.h"
#define HAVE_STYLE_MANAGER __has_include(<KStyleManager>)
#if HAVE_STYLE_MANAGER
#include <KStyleManager>
#endif

#ifdef Q_OS_WINDOWS
#include <windows.h>
#endif

#ifdef Q_OS_ANDROID
Q_DECL_EXPORT
#endif

#if KI18N_VERSION >= QT_VERSION_CHECK(6, 8, 0)
#include <KLocalizedQmlContext>
#endif

using namespace Qt::Literals::StringLiterals;

int main(int argc, char *argv[])
{
#ifdef Q_OS_ANDROID
    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle(QStringLiteral("org.kde.breeze"));
#else
    QApplication app(argc, argv);

#ifndef NDEBUG
    QLoggingCategory::setFilterRules("org.kde.karp*=true"_L1);
    qCDebug(KARP_LOG) << "Debug build";
#endif

#if HAVE_STYLE_MANAGER
    /**
     * trigger initialisation of proper application style
     */
    KStyleManager::initStyle();
#else
    /**
     * For Windows and macOS: use Breeze if available
     * Of all tested styles that works the best for us
     */
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    QApplication::setStyle(QStringLiteral("breeze"));
#endif
#endif

    // Default to org.kde.desktop style unless the user forces another style
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(u"org.kde.desktop"_s);
    }
#endif

#ifdef Q_OS_WINDOWS
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE *outFile = freopen("CONOUT$", "w", stdout);
        if (!outFile)
            qWarning() << "Failed to reopen stdout";
        FILE *errFile = freopen("CONOUT$", "w", stderr);
        if (!errFile)
            qWarning() << "Failed to reopen stderr";
    }

    QApplication::setStyle(QStringLiteral("breeze"));
    auto font = app.font();
    font.setPointSize(10);
    app.setFont(font);
#endif

    KLocalizedString::setApplicationDomain("karp");
    QCoreApplication::setOrganizationName(u"KDE"_s);

    KAboutData aboutData(
        // The program name used internally.
        u"karp"_s,
        // A displayable program name string.
        i18nc("@title", "Karp"),
        // The program version string.
        QStringLiteral(KARP_VERSION_STRING),
        // Short description of what the app does.
        i18n("KDE arranger for PDFs"),
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
    QGuiApplication::setWindowIcon(QIcon::fromTheme(u"org.kde.karp"_s));

    KCrash::initialize();

    QQmlApplicationEngine engine;

    auto config = karpConfig::self();

    qmlRegisterSingletonInstance("org.kde.karp.config", 1, 0, "KarpConf", config);

#if KI18N_VERSION < QT_VERSION_CHECK(6, 8, 0)
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
#else
    engine.rootContext()->setContextObject(new KLocalizedQmlContext(&engine));
#endif
    engine.loadFromModule("org.kde.karp", u"Main"_s);

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
