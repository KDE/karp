// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.settings as KirigamiSettings

KirigamiSettings.CategorizedSettings {
    actions: [
        KirigamiSettings.SettingAction {
            text: i18nc("@title:menu Category in settings", "General")
            actionName: "General"
            icon.name: "settings-configure"
            page: "qrc:/qt/qml/org/kde/deafed/ui/Settings/General.qml"
        },
        KirigamiSettings.SettingAction {
            text: i18nc("@title:menu Category in settings", "PDF tools")
            actionName: "General"
            icon.name: "application-pdf"
            page: "qrc:/qt/qml/org/kde/deafed/ui/Settings/PdfTools.qml"
        }
    ]
}
