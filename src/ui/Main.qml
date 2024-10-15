// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

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

    property int counter: 0

    globalDrawer: Kirigami.GlobalDrawer {
        title: i18n("Simple PDF editor")
        titleIcon: "application-pdf"
        isMenu: Kirigami.Settings.isMobile
        actions: [
            Kirigami.Action {
                text: i18n("About Deaf Ed")
                icon.name: "help-about"
                onTriggered: mainWin.pageStack.pushDialogLayer("qrc:/qt/qml/org/kde/deafed/ui/About.qml")
                // pageStack.push(Qt.resolvedUrl('About.qml'))
            },
            Kirigami.Action {
                text: i18n("Arrange pages")
                icon.name: "view-sort"
                // onTriggered: mainWin.pageStack.pushDialogLayer("qrc:About.qml")
            },
            Kirigami.Action {
                text: i18n("Quit")
                icon.name: "application-exit"
                onTriggered: Qt.quit()
            }
        ]
    }

    contextDrawer: Kirigami.ContextDrawer {
        id: contextDrawer
    }

    pageStack.initialPage: page

    ArrangePage { id: page }
}
