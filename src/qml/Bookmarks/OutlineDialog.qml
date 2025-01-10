// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2025 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import QtQuick.Layouts
import org.kde.karp

Kirigami.Dialog {
    id: outlineDlg

    required property int pageCount
    property var modelIndex: null
    property alias targetPage: targetSpin.value
    property alias bookmarkTitle: titleField.text
    property int whereToAdd: BookmarkModel.Insert.AtEnd

    signal removed()

    title: i18n("Edit Chapter")
    visible: true
    standardButtons: Kirigami.Dialog.NoButton
    implicitWidth: Kirigami.Units.gridUnit * 30
    implicitHeight: Kirigami.Units.gridUnit * 17
    padding: Kirigami.Units.gridUnit

    ColumnLayout {
        id: inner

        Layout.fillWidth: true
        spacing: Kirigami.Units.largeSpacing

        Kirigami.AbstractCard {
            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing
                QQC2.Label {
                    text: i18n("Title")
                }
                QQC2.TextField {
                    id: titleField
                    Layout.fillWidth: true
                }
            }
        }

        Kirigami.AbstractCard {
            contentItem: RowLayout {
                spacing: Kirigami.Units.largeSpacing
                QQC2.Button {
                    text: i18n("first")
                    onClicked: targetSpin.value = 1
                }
                Item { height: 1; Layout.fillWidth: true }
                QQC2.Label { text: i18nc("Edited bookmark/chapter points to page", "points to page") }
                QQC2.SpinBox {
                    id: targetSpin
                    from: 1
                    to: outlineDlg.pageCount
                }
                Item { height: 1; Layout.fillWidth: true }
                QQC2.Button {
                    text: i18n("last")
                    onClicked: targetSpin.value = outlineDlg.pageCount
                }
            }
        }
    }

    customFooterActions: [
        Kirigami.Action {
            visible: whereToAdd === BookmarkModel.Insert.Edit
            text: i18nc("@action:button", "Remove")
            icon.name: "bookmark-remove"
            onTriggered: {
                outlineDlg.removed()
                close()
            }
        },
        Kirigami.Action {
            enabled: titleField.length > 0
            text: whereToAdd === BookmarkModel.Insert.Edit ? i18nc("@action:button", "Save") : i18nc("@action:button", "Add")
            icon.name: whereToAdd === BookmarkModel.Insert.Edit ? "bookmark-edit" : "bookmark-new"
            onTriggered: outlineDlg.accept()
        },
        Kirigami.Action {
            text: i18nc("@action:button", "Cancel")
            icon.name: "dialog-cancel"
            onTriggered: outlineDlg.reject()
        }
    ]

    onClosed: destroy()
}
