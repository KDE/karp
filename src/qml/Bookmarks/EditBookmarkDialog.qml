// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2025 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.karp
import org.kde.kirigami as Kirigami

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
    onClosed: destroy()
    Component.onCompleted: {
        titleField.forceActiveFocus();
    }
    customFooterActions: [
        Kirigami.Action {
            visible: outlineDlg.whereToAdd === BookmarkModel.Insert.Edit
            text: i18nc("@action:button", "Remove")
            icon.name: "bookmark-remove"
            onTriggered: {
                outlineDlg.removed();
                outlineDlg.close();
            }
        },
        Kirigami.Action {
            enabled: titleField.length > 0
            text: outlineDlg.whereToAdd === BookmarkModel.Insert.Edit ? i18nc("@action:button", "Save") : i18nc("@action:button", "Add")
            icon.name: outlineDlg.whereToAdd === BookmarkModel.Insert.Edit ? "bookmark-edit" : "bookmark-new"
            onTriggered: outlineDlg.accept()
        },
        Kirigami.Action {
            text: i18nc("@action:button", "Cancel")
            icon.name: "dialog-cancel"
            onTriggered: outlineDlg.reject()
        }
    ]

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
                    onAccepted: {
                        if (length)
                            outlineDlg.accept();

                    }
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

                Item {
                    Layout.fillWidth: true
                }

                QQC2.Label {
                    text: i18nc("Edited bookmark/chapter points to page", "points to page")
                }

                QQC2.SpinBox {
                    id: targetSpin

                    from: 1
                    to: outlineDlg.pageCount
                }

                Item {
                    Layout.fillWidth: true
                }

                QQC2.Button {
                    text: i18n("last")
                    onClicked: targetSpin.value = outlineDlg.pageCount
                }

            }

        }

    }

}
