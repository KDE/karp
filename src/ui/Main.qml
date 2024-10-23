// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.deafed

Kirigami.ApplicationWindow {
    id: mainWin

    title: PDFED.name

    minimumWidth: Kirigami.Units.gridUnit * 20
    minimumHeight: Kirigami.Units.gridUnit * 20

    onClosing: PDFED.saveWindowGeometry(mainWin)

    Component.onCompleted: PDFED.restoreWindowGeometry(mainWin)

    globalDrawer: Kirigami.GlobalDrawer {
        title: i18n("Simple PDF editor")
        titleIcon: "application-pdf"
        isMenu: Kirigami.Settings.isMobile
        actions: [
            Kirigami.Action {
                text: i18n("About Deaf Ed")
                icon.name: "help-about"
                onTriggered: mainWin.pageStack.pushDialogLayer("qrc:/qt/qml/org/kde/deafed/ui/About.qml")
            },
            Kirigami.Action {
                text: i18n("Settings")
                icon.name: "settings-configure"
                onTriggered: mainWin.pageStack.pushDialogLayer("qrc:/qt/qml/org/kde/deafed/ui/Settings/SettingsPage.qml")
            },
            Kirigami.Action {
                text: i18n("Quit")
                icon.name: "application-exit"
                onTriggered: Qt.quit()
            }
        ]
    }

    pageStack.initialPage: ArrangePage {}
}
