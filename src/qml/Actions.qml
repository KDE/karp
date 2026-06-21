// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2026 by Darshan Phaldesai <dev.darshanphaldesai@gmail.com>

import org.kde.kirigami.actioncollection

ActionCollectionManager {

    ActionCollection {
        name: "org.kde.karp.actions"
        text: i18n("Karp Actions")

        ActionData {
            name: "open_pdf"
            text: i18nc("@action:inmenu", "Add PDF files")
            icon.name: "list-add"
        }

        ActionData {
            name: "clear_all"
            text: i18nc("@action:inmenu", "Clear all files")
            icon.name: "edit-clear-all"
        }
    }
}
