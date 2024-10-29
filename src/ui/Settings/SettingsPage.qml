// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.settings as KirigamiSettings

KirigamiSettings.ConfigurationView {
    window: mainWin // for mobile

    modules: [
        KirigamiSettings.ConfigurationModule {
            moduleId: "general"
            text: i18nc("@title:menu Category in settings", "General")
            icon.name: "settings-configure"
            page: () => Qt.createComponent("org.kde.deafed", "General")
        },
        KirigamiSettings.ConfigurationModule {
            moduleId: "about"
            text: i18nc("@title:menu Category in settings", "PDF tools")
            icon.name: "application-pdf"
            page: () => Qt.createComponent("org.kde.deafed", "PdfTools")
        }
    ]
}
