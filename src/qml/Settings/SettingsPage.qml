// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import org.kde.kirigamiaddons.settings as KirigamiSettings

KirigamiSettings.ConfigurationView {
    modules: [
        KirigamiSettings.ConfigurationModule {
            moduleId: "general"
            text: i18nc("@action:button Category in settings", "General")
            icon.name: "settings-configure"
            page: () => Qt.createComponent("org.kde.karp", "General")
        },
        KirigamiSettings.ConfigurationModule {
            moduleId: "about"
            text: i18nc("@action:button Category in settings", "PDF tools")
            icon.name: "application-pdf"
            page: () => Qt.createComponent("org.kde.karp", "PdfTools")
        },
        KirigamiSettings.ConfigurationModule {
            moduleId: "aboutKarp"
            text: i18nc("@action:button", "About Karp")
            icon.name: "help-about"
            page: () => Qt.createComponent("org.kde.kirigamiaddons.formcard", "AboutPage")
        },
        KirigamiSettings.ConfigurationModule {
            moduleId: "aboutKDE"
            text: i18nc("@action:button", "About KDE")
            icon.name: "kde"
            page: () => Qt.createComponent("org.kde.kirigamiaddons.formcard", "AboutKDEPage")
        }
    ]
}
