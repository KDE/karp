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

        ActionData {
            name: "export"
            text: i18nc("@action:inmenu", "Export Document")
            icon.name: "document-export"
        }

        ActionData {
            name: "multi-delete"
            text: i18nc("@action:intoolbar", "Select pages to delete")
            icon.name: "edit-delete"
            icon.color: "red"
        }

        ActionData {
            name: "multi-rotate"
            text: i18nc("@action:intoolbar", "Select pages to rotate")
            icon.name: "object-rotate-right"
        }

        ActionData {
            name: "multi-move"
            text: i18nc("@action:intoolbar", "Select pages to move")
            icon.name: "transform-move"
        }

        ActionData {
            name: "toggle-bookmarks-pane"
            text: i18nc("@action:intoolbar", "Table of Contents (Bookmarks)")
            icon.name: "bookmark-toolbar"
        }

        ActionData {
            name: "toggle-multi-select"
            text: i18nc("@action:intoolbar", "Multiple pages selection")
            icon.name: "view-pages-overview"
        }

        ActionData {
            name: "toggle-page-labels"
            text: i18nc("@action:intoolbar", "Show page labels")
            icon.name: "label"
        }

        ActionData {
            name: "zoom-in"
            text: i18nc("@action:intoolbar", "Zoom In")
            icon.name: "zoom-in"
        }

        ActionData {
            name: "zoom-out"
            text: i18nc("@action:intoolbar", "Zoom Out")
            icon.name: "zoom-out"
        }
    }
}
