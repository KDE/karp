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
                fromQAction: APP.action("open_pdf")
            },
            Kirigami.Action {
                fromQAction: APP.action("save_pdf")
                text: mainPage.saveAction.text
            },
            Kirigami.Action {
                fromQAction: APP.action("clear_all")
                enabled: mainPage.pdfModel.pageCount
            },
            Kirigami.Action {
                fromQAction: APP.action("options_configure")
            },
            // Kirigami.Action {
            //     fromQAction: APP.action('open_kcommand_bar')
            // },
            Kirigami.Action {
                fromQAction: APP.action('open_about_page')
            },
            Kirigami.Action {
                text: i18n("Quit")
                icon.name: "application-exit"
                onTriggered: Qt.quit()
            }
        ]
    }

    Connections {
        target: APP
        function onWantSettings(): void {
            if (!settings)
                settings = Qt.createComponent("org.kde.deafed", "SettingsPage").createObject(mainWin, { window: mainWin });
            settings.open();
        }
        function onOpenAboutPage(): void {
            mainWin.pageStack.pushDialogLayer("qrc:/qt/qml/org/kde/deafed/qml/About.qml")
        }
    }

    // private
    property SettingsPage settings

    pageStack.initialPage: MainPage {
        id: mainPage
    }
}
