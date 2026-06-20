// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.karp
import org.kde.config as KConfig

Kirigami.ApplicationWindow {
    id: mainWin

    minimumWidth: Kirigami.Units.gridUnit * 20
    minimumHeight: Kirigami.Units.gridUnit * 20

    Component.onCompleted: {
        mainPage.openPDFs(APP.getInitFileList());
    }

    KConfig.WindowStateSaver {
        configGroupName: "main"
    }

    Connections {
        target: APP
        function onWantSettings(): void {
            if (!mainWin.settings)
                mainWin.settings = Qt.createComponent("org.kde.karp", "SettingsPage").createObject(mainWin, {
                    window: mainWin
                });
            settings.open();
        }
    }

    // private
    property SettingsPage settings

    pageStack.initialPage: MainPage {
        id: mainPage
    }
}
