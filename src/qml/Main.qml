// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.deafed

Kirigami.ApplicationWindow {
    id: mainWin

    title: APP.name

    minimumWidth: Kirigami.Units.gridUnit * 20
    minimumHeight: Kirigami.Units.gridUnit * 20

    onClosing: APP.saveWindowGeometry(mainWin)

    Component.onCompleted: {
        APP.restoreWindowGeometry(mainWin)
        mainPage.openPDFs(APP.getInitFileList())
    }

    globalDrawer: Kirigami.GlobalDrawer {
        title: i18n("Simple PDF editor")
        titleIcon: "application-pdf"
        isMenu: Kirigami.Settings.isMobile
        actions: [
            Kirigami.Action {
                text: mainPage.addFilesAction.tooltip
                icon: mainPage.addFilesAction.icon
                onTriggered: mainPage.addFilesAction.trigger()
            },
            Kirigami.Action {
                enabled: mainPage.saveAction.enabled
                icon: mainPage.saveAction.icon
                text: mainPage.saveAction.text
                onTriggered: mainPage.saveAction.trigger()
            },
            Kirigami.Action {
                enabled: mainPage.pdfModel.pageCount
                text: i18n("Clear all files")
                icon.name: "edit-clear-all"
                onTriggered: mainPage.clearAll()
            },
            Kirigami.Action {
                text: i18n("Settings")
                icon.name: "settings-configure"
                onTriggered: {
                    if (!settings)
                        settings = Qt.createComponent("org.kde.deafed", "SettingsPage").createObject(mainWin);
                    settings.open();
                }
            },
            Kirigami.Action {
                text: i18n("About Deaf Ed")
                icon.name: "help-about"
                onTriggered: mainWin.pageStack.pushDialogLayer("qrc:/qt/qml/org/kde/deafed/qml/About.qml")
            },
            Kirigami.Action {
                text: i18n("Quit")
                icon.name: "application-exit"
                onTriggered: Qt.quit()
            }
        ]
    }

    // private
    property SettingsPage settings

    pageStack.initialPage: MainPage {
        id: mainPage
    }
}
