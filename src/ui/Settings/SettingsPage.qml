// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.settings as KirigamiSettings
// import org.kde.deafed

KirigamiSettings.CategorizedSettings {
    actions: [
        KirigamiSettings.SettingAction {
            text: i18nc("@title:menu Category in settings", "Applications")
            actionName: "General"
            icon.name: "application-pdf"
            page: "qrc:/qt/qml/org/kde/deafed/ui/Settings/PdfTools.qml"
        },
        KirigamiSettings.SettingAction {
            text: i18nc("@title:menu Category in settings", "About")
            actionName: "General"
            icon.name: "help-about"
            page: "qrc:/qt/qml/org/kde/deafed/ui/About.qml"
        }
    ]
}
