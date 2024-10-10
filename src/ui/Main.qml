// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.deafed

Kirigami.ApplicationWindow {
    id: mainWin

    title: "DeaFEd"

    minimumWidth: Kirigami.Units.gridUnit * 20
    minimumHeight: Kirigami.Units.gridUnit * 20

    onClosing: App.saveWindowGeometry(mainWin)

    onWidthChanged: saveWindowGeometryTimer.restart()
    onHeightChanged: saveWindowGeometryTimer.restart()
    onXChanged: saveWindowGeometryTimer.restart()
    onYChanged: saveWindowGeometryTimer.restart()

    Component.onCompleted: App.restoreWindowGeometry(mainWin)

    // This timer allows to batch update the window size change to reduce
    // the io load and also work around the fact that x/y/width/height are
    // changed when loading the page and overwrite the saved geometry from
    // the previous session.
    Timer {
        id: saveWindowGeometryTimer
        interval: 1000
        onTriggered: App.saveWindowGeometry(mainWin)
    }

    property int counter: 0

    globalDrawer: Kirigami.GlobalDrawer {
        title: i18n("Simple PDF editor")
        titleIcon: "application-pdf"
        isMenu: Kirigami.Settings.isMobile
        actions: [
            Kirigami.Action {
                text: i18n("About DeaFEd")
                icon.name: "help-about"
                onTriggered: mainWin.pageStack.pushDialogLayer("qrc:/qt/qml/org/kde/deafed/contents/ui/About.qml")
                // pageStack.push(Qt.resolvedUrl('About.qml'))
            },
            Kirigami.Action {
                text: i18n("Reorder pages")
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

    Kirigami.Page {
        id: page

        title: i18n("Reorder PDF Pages")

        ColumnLayout {
            width: page.width

            anchors.centerIn: parent
        }
    }
}
