// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.karp

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
                fromQAction: APP.action('open_pdf')
            },
            Kirigami.Action {
                fromQAction: APP.action('save_pdf')
                text: mainPage.saveAction.text
            },
            Kirigami.Action {
                fromQAction: APP.action('clear_all')
                enabled: mainPage.pdfModel.pageCount
            },
            Kirigami.Action {
                fromQAction: APP.action('options_configure')
            },
            // Kirigami.Action {
            //     fromQAction: APP.action('open_kcommand_bar')
            // },
            Kirigami.Action {
                fromQAction: APP.action('open_about_page')
            },
            Kirigami.Action {
                fromQAction: APP.action('open_about_kde_page')
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
                settings = Qt.createComponent("org.kde.karp", "SettingsPage").createObject(mainWin, { window: mainWin });
            settings.open();
        }
        function onOpenAboutPage(): void {
            const aboutDlg = pageStack.pushDialogLayer(Qt.createComponent("org.kde.kirigamiaddons.formcard", "AboutPage"), {
                width: mainPage.width
            }, {
                width: Math.min(Kirigami.Units.gridUnit * 40, mainWin.width * 0.9),
                height: Math.min(Kirigami.Units.gridUnit * 30, mainWin.height * 0.8)
            });
        }
        function onOpenAboutKDEPage(): void {
            const aboutKdeDlg = pageStack.pushDialogLayer(Qt.createComponent("org.kde.kirigamiaddons.formcard", "AboutKDEPage"), {
                width: mainPage.width
            }, {
                width: Math.min(Kirigami.Units.gridUnit * 40, mainWin.width * 0.9),
                height: Math.min(Kirigami.Units.gridUnit * 30, mainWin.height * 0.8)
            });
        }
    }

    // private
    property SettingsPage settings

    pageStack.initialPage: MainPage {
        id: mainPage
    }
}
